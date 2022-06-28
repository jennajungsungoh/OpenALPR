from django.contrib.auth import authenticate, login
from django.shortcuts import render, redirect
from login.forms import UserForm
#from duo_universal_auth.middleware import


def signup(request):
    if request.method == "POST":
        form = UserForm(request.POST)
        if form.is_valid():
            form.save()
            username = form.cleaned_data.get('username')
            raw_password = form.cleaned_data.get('password')
            user = authenticate(username=username, password=raw_password)  # 사용자 인증
            login(request, user)  # 로그인
            #DuoUniversalAuthMiddleware(settings.DUO_UNIVERSAL_AUTH, request):
            return redirect('/')
    else:
        form = UserForm()
    return render(request, 'login/signup.html', {'form': form})
