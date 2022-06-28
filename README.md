# OpenALPR (Team 2, 2022-Security Specialist)

## Python Environment
```
$ cd studio/Script
$ activate
```
## Set Env & DB Migration
```
$ cd studio
$ pip install -r requirement.txt

$ cd
$ python .\manage.py makemigrations
$ python .\manage.py migrate   
```

## Start Server
```
$ cd stduio/webapp/
$ python .\manage.py runserver 0.0.0.0:8000

#loging page
http://127.0.0.1:8000/
or
http://127.0.0.1:8000/login/login

-------------
http://127.0.0.1:8000/alpr/
```

## Admin
```
  http://127.0.0.1:8000/admin
  ID: admin
  PW: ahnlab
```

## Execute Plate lookup server 
```
> cd OpenALPR
> run setup.bat
> cd x64/Release
> run loaddb.exe
> run server.exe
```