from django.shortcuts import render
from django.http import HttpResponse
from django.http import StreamingHttpResponse
from django.shortcuts import redirect
from django.core.files.base import ContentFile
from django.contrib.auth.decorators import login_required


from django.views.decorators import gzip
from django.core import serializers
from django.db.models import Avg, F, Count, Min, Value, CharField, Func
from django.db.models.functions import Concat
from . import models

import threading
import cv2
import numpy
from openalpr import Alpr

import os
import time
import random
import shutil 
import base64

import json
from django.http.response import JsonResponse


from django.conf import settings

import socket
import ssl
import json

HOST = 'localhost'
PORT = 2222
server_sni_hostname = 'example.com'
server_cert = 'rootca.crt'

json_key_list = ['PlateNumber' , 'Status' , 'RegistrationExpiration', 'OwnerName' , 'OwnerBirth' , 'OwnerAddress' , 'OwnerZipCode' , 'VehicleYearOfManufacture' , 'VehicleMake' , 'VehicleModel' , 'VehicleColor']


rootpath = os.path.dirname(os.path.abspath(__file__)) + os.path.sep
dllabspath = rootpath + "../../openalpr64-sdk-4.1.1"
os.add_dll_directory(dllabspath) 
print(dllabspath) 
stream = None

font = cv2.FONT_HERSHEY_COMPLEX_SMALL


class Round(Func): 
    function = 'ROUND'
    template='%(function)s(%(expressions)s, 2)'
  


class VideoStream(object):
    def __init__(self, playback=False, pid=None, request=None):
        self._avgdur=0
        self._fpsstart=0
        self._avgfps=0
        self._fps1sec=0
        self._pid=pid
        self.recognized_plate_numbers = []

        if request is not None:
            self._session_key = request.session.session_key
            self.user = request.user
        else : 
            self._session_key = 0
        
        
        self.remove_vehicle_by_session()

        if playback: 
            self.TEST_VIDEO_FILE_PATH = settings.MEDIA_ROOT + settings.MEDIA_URL + self.user.username + '/' + self._pid 
            self.video = cv2.VideoCapture(self.TEST_VIDEO_FILE_PATH)
        else:
            self.video = cv2.VideoCapture(0)
 
        OPENALPR_CONFIG = dllabspath + '/openalpr.conf'     #'/path/to/openalpr.conf'
        RUNTIME_DATA_PATH = dllabspath + '/runtime_data'   #'/path/to/runtime_data'
        ALPR_COUNTRY = 'us' 

        
        self.alpr = Alpr(ALPR_COUNTRY, OPENALPR_CONFIG, RUNTIME_DATA_PATH)
 
        (self.grabbed, self.frame) = self.video.read()
        threading.Thread(target=self.update, args=()).start()
 
    def __del__(self):
        self.video.release()
        print("destroy.............")

    def get_frame(self, frame=None):
        try:
            if frame is None:
                image = self.frame
                _, jpeg = cv2.imencode('.jpg', image)
            else :
                image = frame
                _, jpeg = cv2.imencode('.jpg', image)
        except:
            print("close connection")
            # connection close
            self.conn.close() 
            return None, None

        return image, jpeg.tobytes()

    def get_framenumber(self):
        return self.framenumber

    def clear_text(self): 
        self.x1=0
        self.y1=0
        self.x2=0 
        self.y2=0
        self.f_text=''
        self.fps_text=''
     
    def avgdur(self, newdur):  
        self._avgdur = 0.98 * self._avgdur + 0.02 * newdur 
        return self._avgdur

    def avgfps(self): 
        clock = time.perf_counter() * 1000  
        if clock - self._fpsstart > 1000 :
            self._fpsstart = clock
            self._avgfps = 0.7 * self._avgfps + 0.3 * self._fps1sec
            self._fps1sec = 0
         
        self._fps1sec+=1 
        return self._avgfps

    def remove_vehicle_by_session(self): 
        models.Vehicle.objects.filter(filename=self._pid, session_key=self._session_key).delete()
 

    def add_database(self, pn, cd, frame_raw, w, h, vehicle_info = None): 
        # Saving the information in the database

        cropped_size = 30
        try :
            if self.x1-cropped_size<0: x1 = 0 
            else: x1 = self.x1 - cropped_size 
            if self.y1-cropped_size<0: y1 = 0 
            else: y1 = self.y1 - cropped_size
            if self.x2+cropped_size>w: x2 = w 
            else: x2 = self.x2 + cropped_size
            if self.y2+cropped_size>h: y2 = h 
            else: y2 = self.y2 + cropped_size

            cropped_img = frame_raw[y1:y2, x1:x2]
            # print("x:{}-{}, y:{}-{} size:{} c_size:{}".format(self.x1, self.x2, self.y1, self.y2, frame_raw.size, cropped_img.size))
            _, buf = cv2.imencode('.jpg', cropped_img)  
            content = ContentFile(buf.tobytes())
        except Exception as e:
            print('add database error: %s' % (e)) 
            pass

        if vehicle_info is not None:
            plate_number_likely = vehicle_info.get('PlateNumber')
            state = vehicle_info.get('Status')
            regist_exp = vehicle_info.get('RegistrationExpiration')
            owner = vehicle_info.get('OwnerName')
            owner_birth = vehicle_info.get('OwnerBirth')
            owner_addr = vehicle_info.get('OwnerAddress')
            owner_zip = vehicle_info.get('OwnerZipCode')
            yom= vehicle_info.get('VehicleYearOfManufacture')
            maker = vehicle_info.get('VehicleMake')
            model= vehicle_info.get('VehicleModel')
            color = vehicle_info.get('VehicleColor')
        
        # print(plate_number_likely)
        vehicle = models.Vehicle(
            filename = self._pid, 
            plate_number = pn,
            plate_number_likely = plate_number_likely,
            confidence = cd,
            frame_no = self.framenumber,
            session_key = self._session_key,
            user = self.user,   
            captured_frame = content,
            status = vehicle_info.get('Status'),
            regist_exp = regist_exp,
            owner = owner,
            owner_birth = owner_birth,
            owner_addr = owner_addr,
            owner_zip = owner_zip,
            yom = yom,
            maker = maker,
            model= model,
            color = color,
        ) 
        vehicle.captured_frame.name = "{}.jpg".format(self.framenumber)
        vehicle.save()

    def is_duplicated(self, pn):
        if pn in self.recognized_plate_numbers: 
            # print("already recognized..{}".format(pn))
            return True
        else: 
            # print("new one - {}".format(pn))
            self.recognized_plate_numbers.append(pn)
            return False 

    def query_and_save(self, pn, cd, width, height, frame_raw): 
        vehicle_data = []
        vehicle_data_json = {} 

        if not self.is_duplicated(pn):  
            # todo : connet with server(ssl) 
            sendMsgHdr=(len(pn)+1)
            sendMsgHdr2=sendMsgHdr.to_bytes(2, 'big')
            self.conn.sendall(sendMsgHdr2) 
            #print('Data : {} , Data Length : {}'.format(pn, sendMsgHdr2))

            self.conn.sendall(pn.encode('utf-8')) 
            
            data = self.conn.recv(1024)
            
            if data != b'\x00\x00' :
                data = self.conn.recv(1024)
                data=data.decode('utf-8')
                vehicle_data = data.split('\n') 
                vehicle_data_json = dict( zip(json_key_list, vehicle_data) )
                json.dumps(vehicle_data_json)        
            
            if vehicle_data_json is not None:
                self.add_database(pn, cd, frame_raw, width, height, vehicle_data_json)
            else :
                self.add_database(pn, cd, frame_raw, width, height)

    def update(self): 
        self.clear_text()
        self.framenumber = 0
        height = int(self.video.get(4))

        context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile=server_cert)
        context.check_hostname = False
        context.load_verify_locations('rootca.crt')


        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.conn = context.wrap_socket(s, server_side=False, server_hostname=server_sni_hostname)
        self.conn.connect((HOST, PORT))
        print("conn...")
       
        while True:
            try:
                start = time.perf_counter() 
                (self.grabbed, self.frame) = self.video.read()

                if self.frame is not None:
                    clear_frame = self.frame.copy()
                else:
                    break
                
                cv2.rectangle(self.frame, (self.x1, self.y1), (self.x2, self.y2), (0, 255, 0), 2)
                cv2.putText(self.frame, self.f_text, (self.x1, self.y1-30), font, 1, (0, 255, 0), 0, cv2.LINE_AA)
                cv2.putText(self.frame, self.fps_text, (0, height-10), font, 0.7, (0, 255, 0), 0, cv2.LINE_AA)

                frame_raw, frame_byte = self.get_frame(clear_frame)
                results = self.alpr.recognize_array(frame_byte)
    
    
    
                if results['data_type'] == 'alpr_results': 
                    if results['results']:
                        pn = results['results'][0]['candidates'][0]['plate'] 
                        cd = results['results'][0]['candidates'][0]['confidence']
                        
                        self.f_text = '{} ({:.2f}%)'.format(pn, cd) 
                        self.x1 = results['results'][0]['coordinates'][0]['x']
                        self.y1 = results['results'][0]['coordinates'][0]['y'] 
                        self.x2 = results['results'][0]['coordinates'][2]['x']
                        self.y2 = results['results'][0]['coordinates'][2]['y']

                        width = results['img_width']
                        height = results['img_height']
                        print('Plate: {} ({:.2f}%) {}x{}- x:{}-{} y:{}-{}'.format(pn, cd, width, height, self.x1, self.x2, self.y1, self.y2))
                        
                        self.query_and_save(pn, cd, width, height, frame_raw)

                        
                    else :    
                        self.clear_text() 
    
                dur = time.perf_counter() - start
            
                # print (dur*1000)             
                self.fps_text = 'avg time per frame {:.5f} ms. fps {:.2f}. frameno = {}'.format(self.avgdur(dur*1000), self.avgfps(), self.framenumber)
                self.framenumber+=1
            except Exception as e:
                print('update error: %s' % (e)) 
                self.conn.close()  

def remove_video():
    for f in models.Document.objects.all():
        print(f.uploadedFile.url)
        os.remove(rootpath+'..'+f.uploadedFile.url)
        f.delete()

def remove_vehicle(self):
        models.Vehicle.objects.all().delete()

def gen_stream(stream) :
    while True:
        _, frame = stream.get_frame()
        if frame is not None:
            yield(b'--frame\r\n' 
                  b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n\r\n') 
        else :
            break

def gen_data(stream) :
    while True:
        fn = stream.get_framenumber()
        data={} 
        data['framenumber'] = fn
       
        # yield(b'--fn\r\n'
        #       b'Content-Type: application/json\r\n\r\n' + json.dumps(data).encode('utf-8') + b'\r\n\r\n') 
        yield 'data: %s\n\n' % fn
 
# Create your views here.'  
@gzip.gzip_page   
@login_required(login_url='/login/login')
def playback(request):  
    try: 
        pid = request.GET['pid']
        stream = VideoStream(playback = True, pid = pid, request = request)
        return StreamingHttpResponse(gen_stream(stream), content_type="multipart/x-mixed-replace;boundary=frame") 
        # return StreamingHttpResponse(gen_data(stream), content_type="text/event-stream") 
    except Exception as e:
        print('remove_vehicle_history error: %s' % (e)) 
        pass

@gzip.gzip_page
@login_required(login_url='/login/login')
def webcam(request):
    try:
        stream = VideoStream(playback = False)
        return StreamingHttpResponse(gen_stream(stream), content_type="multipart/x-mixed-replace;boundary=frame") 
    except:  
        print("error")
        pass 

@login_required(login_url='/login/login')
def get_vehicle(request): 
    pid = request.GET['pid']
    session_key = request.session.session_key

    # vehicles = models.Vehicle.objects.filter(filename=pid).order_by('-detected_at')
    vehicles = models.Vehicle.objects.filter(filename=pid, session_key=session_key).values('detected_at__hour', 'detected_at__minute', 'detected_at__second', 'plate_number').annotate(
            confidence_avg=Round(Avg('confidence'))).order_by(
                '-detected_at__hour', '-detected_at__minute', '-detected_at__second', '-confidence_avg').annotate(
                    time=Concat('detected_at__hour', Value(':'), 'detected_at__minute', Value(':'), 'detected_at__second', output_field=CharField())
                ).values('time','plate_number','confidence_avg')
    
    # vehicles_list = serializers.serialize('json', vehicles,  fields=('plate_number',))
    # return HttpResponse(vehicles, content_type="text/json-comment-filtered")
    # return HttpResponse(vehicles, content_type='application/json')
    return JsonResponse(list(vehicles), safe=False)

@login_required(login_url='/login/login')
def post_frame(request):
    while True : 
        request.session['frameno'] = request.session['frameno']+10
        time.sleep(1)
        if request.session['frameno'] == 100:
            break
    frameno = request.session['frameno'] 
    return HttpResponse(json.dumps({'fn': frameno}), content_type='application/json')

@login_required(login_url='/login/login')
def get_frame(request): 
    try:
        frameno = request.session['frameno']
    except: 
        print('get_frame : error')
        pass
    return HttpResponse(json.dumps({'fn': frameno}), content_type='application/json')

@login_required(login_url='/login/login')
def index(request): 
    
    if request.method == "POST":
        mode = request.POST["mode"] 
        pid = request.POST["pid"]
        
        return render(request, 'alpr/index.html',  
        {'mode':mode, 'pid':pid}) 
    else: 
        documents = models.Document.objects.all()
        return render(request, 'alpr/index.html', context = {
        "files": documents})
          
@login_required(login_url='/login/login')
def get_captured_vehicle(request):
    plate_number = request.POST['plate_number']
    filename = request.POST['filename']
    user = request.user

    vehicles = models.Vehicle.objects.filter(plate_number=plate_number, filename=filename, user=user).order_by('-confidence').values().first()
    # vehilcle = serializers.serialize("json", vehicles)
    # print(vehilcle) 
    # print(list(vehicles))
    return JsonResponse(vehicles, safe=False)

@login_required(login_url='/login/login')
def get_captured_plate(request):
    plate_number = request.POST['plate_number']
    filename = request.POST['filename']
    user = request.user

    vehicles = models.Vehicle.objects.filter(plate_number=plate_number, filename=filename, user=user).order_by('-confidence').first()
    plate_path = settings.MEDIA_ROOT + os.path.sep + vehicles.captured_frame.name
    
    image = cv2.imread(plate_path)
    cv2.putText(image, "frame:{}".format(vehicles.frame_no), (10, 20), font, 0.6, (0, 0, 255), 0, cv2.LINE_AA)
    encode_image = cv2.imencode('.jpg', image)
    binary = base64.b64encode(encode_image[1]).decode('utf-8')
 
    
    return HttpResponse(binary)
    # return HttpResponse(json.dumps({'pid': plate_number}), content_type='application/json')

@login_required(login_url='/login/login')
def play(request): 
    pid = request.POST['pid']
    return render(request, 'index.html', ctx)

@login_required(login_url='/login/login')
def remove(request):
    id = request.GET['id']
    filepath = settings.BASE_DIR 
    url = models.Document.objects.get(id=id).uploadedFile.url

    try:
        models.Document.objects.filter(id=id).delete()
        print(rootpath+'../'+url)
        os.remove(rootpath+'../'+url)
    except:
        print("remove error")
  
    documents = models.Document.objects.all()
    return redirect('/alpr') 

@login_required(login_url='/login/login')
def remove_vehicle_history(request):
    user = request.user
    filename = request.GET['filename']
    image_path = settings.MEDIA_ROOT + os.path.sep + 'media' + os.path.sep + user.username + os.path.sep + filename.rsplit('.')[0]
    try:
        models.Vehicle.objects.filter(user=user, filename=filename).delete()
        shutil.rmtree(image_path)
    except Exception as e:
        print('remove_vehicle_history error: %s' % (e)) 
    return redirect('/alpr') 

@login_required(login_url='/login/login')
def upload_view(request):
    documents = models.Document.objects.all()
  
    return render(request, "alpr/upload.html", context = {
        "files": documents
    })  

@login_required(login_url='/login/login')
def upload(request):
    if request.method == "POST":
        # Fetching the form data
        uploadedFile = request.FILES["uploadedFile"]
        user = request.user

        # Saving the information in the database
        document = models.Document(
            user = user,
            uploadedFile = uploadedFile
        )
        document.save()
    return redirect('/alpr') 
 

def config(request):
    # todo action..
    print("do something...")
    return redirect('/alpr') 