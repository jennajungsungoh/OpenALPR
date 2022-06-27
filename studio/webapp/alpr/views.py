from django.shortcuts import render
from django.http import HttpResponse
from django.http import StreamingHttpResponse
from django.views.decorators import gzip
from django.core import serializers
from django.db.models import Avg, F, Count, Min, Value, CharField, Func
from django.db.models.functions import Concat
from . import models

import threading
import cv2
import numpy
from alprstream import AlprStream
from openalpr import Alpr, VehicleClassifier

import os
import time
import random

import json
from django.http.response import JsonResponse


from django.conf import settings

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
        if request is not None:
            self.session_key = request.session.session_key
            self.user = request.user
        else : 
            self.session_key = 0
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
        # print(dllabspath)

    def __del__(self):
        self.video.release()

    def get_frame(self):
        try:
            image = self.frame
            _, jpeg = cv2.imencode('.jpg', image)
        except:
            print("exception...")
            # self.remove_vehicle_by_session()

        return jpeg.tobytes()

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
        models.Vehicle.objects.filter(session_key=self.session_key).delete()


    def add_database(self, pn, cd):
        # Saving the information in the database
        vehicle = models.Vehicle(
            filename = self._pid,
            plate_number = pn,
            confidence = cd,
            frame_no = self.framenumber,
            session_key = self.session_key,
            user = self.user
        )
        vehicle.save()

    def update(self): 
        self.clear_text()
        self.framenumber = 0
        height = int(self.video.get(4))
       
        while True:
            start = time.perf_counter() 
            (self.grabbed, self.frame) = self.video.read()
            cv2.rectangle(self.frame, (self.x1, self.y1), (self.x2, self.y2), (0, 255, 0), 2)
            cv2.putText(self.frame, self.f_text, (self.x1, self.y1-30), font, 1, (0, 255, 0), 0, cv2.LINE_AA)
            cv2.putText(self.frame, self.fps_text, (0, height-10), font, 0.7, (0, 255, 0), 0, cv2.LINE_AA)
 
            results = self.alpr.recognize_array(self.get_frame())

            if results['data_type'] == 'alpr_results': 
                if results['results']:
                    pn = results['results'][0]['candidates'][0]['plate'] 
                    cd = results['results'][0]['candidates'][0]['confidence']
                     
                    print('Plate: {} ({:.2f}%)'.format(pn, cd))
                    # print(results)
                     
                    self.f_text = '{} ({:.2f}%)'.format(pn, cd) 
                    self.x1 = results['results'][0]['coordinates'][0]['x']
                    self.y1 = results['results'][0]['coordinates'][0]['y'] 
                    self.x2 = results['results'][0]['coordinates'][2]['x']
                    self.y2 = results['results'][0]['coordinates'][2]['y']
                    width = results['img_width']
                    height = results['img_height']
 
                    

                    # todo : connet with server(ssl)
                    # 하헌진.
                    self.add_database(pn, cd)
                else :  
                    self.clear_text() 
 
            dur = time.perf_counter() - start
          
            # print (dur*1000)             
            self.fps_text = 'avg time per frame {:.5f} ms. fps {:.2f}. frameno = {}'.format(self.avgdur(dur*1000), self.avgfps(), self.framenumber)
            self.framenumber+=1
            
            # print(self.fps_text)

def remove_video():
    for f in models.Document.objects.all():
        print(f.uploadedFile.url)
        os.remove(rootpath+'..'+f.uploadedFile.url)
        f.delete()

def remove_vehicle(self):
        models.Vehicle.objects.all().delete()

def gen_stream(stream) :
    while True:
        frame = stream.get_frame()
        yield(b'--frame\r\n' 
              b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n\r\n') 

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
def playback(request):  
    try: 
        pid = request.GET['pid']
        stream = VideoStream(playback = True, pid = pid, request = request)
        return StreamingHttpResponse(gen_stream(stream), content_type="multipart/x-mixed-replace;boundary=frame") 
        # return StreamingHttpResponse(gen_data(stream), content_type="text/event-stream") 
    except: 
        print("error")
        pass

@gzip.gzip_page
def webcam(request):
    try:
        stream = VideoStream(playback = False)
        return StreamingHttpResponse(gen_stream(stream), content_type="multipart/x-mixed-replace;boundary=frame") 
    except: 
        print("error")
        pass 


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

def post_frame(request):
    while True : 
        request.session['frameno'] = request.session['frameno']+10
        time.sleep(1)
        if request.session['frameno'] == 100:
            break
    frameno = request.session['frameno'] 
    return HttpResponse(json.dumps({'fn': frameno}), content_type='application/json')

def get_frame(request): 
    try:
        frameno = request.session['frameno']
    except: 
        print('get_frame : error')
        pass
    return HttpResponse(json.dumps({'fn': frameno}), content_type='application/json')

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
          
 
def play(request): 
    pid = request.POST['pid']
    return HttpResponse(json.dumps({'pid': pid}), content_type='application/json')
 


def remove(request):
    id = request.GET['id']
    print(id)
    filepath = settings.BASE_DIR 
    url = models.Document.objects.get(id=id).uploadedFile.url

    os.remove(rootpath+'../'+url)
    models.Document.objects.filter(id=id).remove()
  
    documents = models.Document.objects.all()
    return render(request, 'alpr/index.html', context = {
        "files": documents})

def upload_view(request):
    documents = models.Document.objects.all()
 
    return render(request, "alpr/upload.html", context = {
        "files": documents
    }) 

def upload(request):
    if request.method == "POST":
        # Fetching the form data
        uploadedFile = request.FILES["uploadedFile"]
        filename = request.FILES["uploadedFile"].name
        user = request.user

        # remove all data 
        #remove_all()

        # Saving the information in the database
        document = models.Document(
            user = user,
            uploadedFile = uploadedFile
        )
        document.save()
        
    return render(request, 'alpr/index.html', {
        'mode':0, 'pid':filename
    })
