from faker import Faker
from faker.generator import random
from faker_vehicle import VehicleProvider
from datetime import datetime
fake = Faker('en_US')
fake.add_provider(VehicleProvider)
Faker.seed(0)
 
for i in range(150):
 if i == 0:
  plate="LKY1360"
  rnum=2
 elif i == 1:
  plate="HHF6697"
  rnum=5
 elif i == 2:
  plate="GVP9164"
  rnum=8
 elif i == 3:
  plate="LBX9051"  
  rnum=10
 elif i == 4:
  plate="06062"
  rnum=10  
 elif i == 5:
  plate="LBV6157"
  rnum=10  
 elif i == 6:
  plate="LVH6056"
  rnum=10  
 elif i == 7:
  plate="ZPV5837" 
  rnum=10  
 elif i == 8:
  plate="ZDE1985"
  rnum=10
 elif i == 9:
  plate="JBK6142"
  rnum=2
 elif i == 10:
  plate="ZRP9348"
  rnum=4
 elif i == 11:
  plate="MSJWHY"
  rnum=5
 elif i == 12:
  plate="F0RNER2"
  rnum=8
 elif i == 13:
  plate="LVM6107"  
  rnum=10
 elif i == 14:
  plate="KKM1789"
  rnum=10  
 elif i == 15:
  plate="KFD6960"
  rnum=10  
 elif i == 16:
  plate="ZLD0922"
  rnum=10  
 elif i == 17:
  plate="KYT3950" 
  rnum=10  
 elif i == 18:
  plate="LKS9443"
  rnum=10
 elif i == 19:
  plate="YDS5255"
  rnum=2
 elif i == 50:
  plate="KGJ8487"
  rnum=4
 elif i == 51:
  plate="ZNS2724"
  rnum=5
 elif i == 52:
  plate="ZNM2197"
  rnum=8
 elif i == 53:
  plate="ZGS7240"  
  rnum=10
 elif i == 54:
  plate="GXV2941"
  rnum=2  
 elif i == 55:
  plate="LPH0511"
  rnum=4 
 elif i == 56:
  plate="KYG9827"
  rnum=5  
 elif i == 57:
  plate="YAM0025" 
  rnum=8  
 elif i == 58:
  plate="FJC6534"
  rnum=10
 elif i == 59:
  plate="CPTMGN2"
  rnum=2 
 elif i == 60:
  plate="HYY8868"
  rnum=4
 elif i == 61:
  plate="JHG7802"
  rnum=5
 elif i == 62:
  plate="BETELU"
  rnum=8
 elif i == 63:
  plate="LBX9129"  
  rnum=10
 elif i == 64:
  plate="LDX1620"
  rnum=10  
 elif i == 65:
  plate="KCS6722"
  rnum=2  
 elif i == 66:
  plate="KYR3878"
  rnum=4  
 elif i == 67:
  plate="JJC5503" 
  rnum=5
 elif i == 68:
  plate="LMC5535" 
  rnum=8
 elif i == 91:
  plate="JXK1447"
  rnum=5
 elif i == 92:
  plate="KXZ7041"
  rnum=8
 elif i == 93:
  plate="KZS9188"  
  rnum=10
 elif i == 94:
  plate="K66319K"
  rnum=2  
 elif i == 95:
  plate="LNF6519"
  rnum=4 
 elif i == 96:
  plate="LDD4877"
  rnum=5  
 elif i == 97:
  plate="HMZ2628" 
  rnum=8  
 elif i == 98:
  plate="JDK7141"
  rnum=10
 else:
  plate=fake.license_plate().replace(" ","")
  plate=plate.replace("-","")
  rnum=fake.pyint(1,100)
 output=plate+"\n"
 if rnum < 3:
  status="Owner Wanted"
 elif rnum <6: 
  status="Unpaid Fines - Tow"
 elif rnum <9: 
  status="Stolen"
 else:
  status="No Wants / Warrants"
 output+=status+"\n"
 #output+=fake.date_this_year().strftime("%m/%d/%Y")+"\n"
 output+=fake.date_between_dates(date_start=datetime(2022,1,1), date_end=datetime(2024,5,1)).strftime("%m/%d/%Y")+"\n"
 output+=fake.name()+"\n"
 #output+=fake.date_of_birth().strftime("%m/%d/%Y")+"\n"
 output+=fake.date_between_dates(date_start=datetime(1932,1,1), date_end=datetime(2004,1,1)).strftime("%m/%d/%Y")+"\n"
 output+=fake.address()+"\n"
 output+=fake.vehicle_year()+"\n"
 output+=fake.vehicle_make()+"\n"
 output+=fake.vehicle_model()+"\n"
 output+=fake.safe_color_name()+"$"
 print(output, end='')
