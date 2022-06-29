# OpenALPR (Team 2, 2022-Security Specialist)

# Prerequisites
Python 3.10+ (tested against 3.10)


# Step 1
Make python env
```
$ python -m venv team2 
$ cd team2\Script
$ activate // (for vs code)
```
# Step 2 
Install requirements
```
$ cd OpenAlpr\studio
$ pip install -r .\requirement.txt 
```


## Python Environment
```
## Internal dns settting. https://nip.io/   (One time only)
$ cd nip.io-master
$ build_and_run_docker.sh

$ cd studio/Script
$ activate
```
## Set Env & DB Migration
```
$ cd studio
$ pip install -r requirement.txt


$ cd webapp
$ change IPADDRESS = '10.58.7.138' to correct ipaddress in settings.py

$ cd
$ python .\manage.py makemigrations
$ python .\manage.py migrate   
```


## Start Server
```
$ cd stduio/webapp/
$ python .\manage.py runsslserver 0.0.0.0:8000 --certificate django.crt --key django.key

#loging page
https://{ipaddress}.nip.io:8000/
ex>  if ipadress is 10.58.7.138 https://10.58.7.138.nip.io:8000/
or
https://{ipaddress}.nip.io:8000/login/login
ex>  if ipadress is 10.58.7.138 https://10.58.7.138.nip.io:8000/login/login
-------------
https://{ipaddress}.nip.io:8000/alpr/
ex>  if ipadress is 10.58.7.138 https://10.58.7.138.nip.io:8000/alpr/
```

## Admin
```
https://{ipaddress}.nip.io:8000/admin
ex>  if ipadress is 10.58.7.138 https://10.58.7.138.nip.io:8000/admin/
  ID: admin
  PW: ahnlab
```

## Execute Plate lookup server 
```
> cd plateServer
> run setup.bat
> cd x64/Release
> run loaddb.exe
> run server.exe
```
