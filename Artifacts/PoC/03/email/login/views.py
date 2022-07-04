from django.contrib.auth import authenticate, login
from django.shortcuts import render, redirect
from django.contrib.auth import views as auth_views
from django.urls import reverse_lazy
from .forms import UserForm, PasswordResetForm, FakeUserForm, EmailInfoForm
from django.core.mail.message import EmailMessage


def index(request):
    form = EmailInfoForm()
    return render(request, 'login/send_email.html', {'form': form})

def send_email(request):
    if request.method == "POST":
        form = EmailInfoForm(request.POST)
        if form.is_valid():
            EmailMessage(subject=form.cleaned_data.get('subject'), body=form.cleaned_data.get('message'), to=(form.cleaned_data.get('to').split(';')), from_email=form.cleaned_data.get('from_email')).send()
            return redirect('/login/password_reset/done/')
    else:
        form = EmailInfoForm()
    return render(request, 'login/send_email.html', {'form': form})


def fakelogin(request):
    if request.method == "POST":
        form = FakeUserForm(request.POST)
        if form.is_valid():
            post = form.save(commit=False)
            return render(request, 'login/logininfo.html', {'post': post})
    else:
        form = FakeUserForm()
    return render(request, 'login/fakelogin.html', {'form': form})



def signup(request):
    if request.method == "POST":
        form = UserForm(request.POST)
        if form.is_valid():
            form.save()
            username = form.cleaned_data.get('username')
            raw_password = form.cleaned_data.get('password1')
            user = authenticate(username=username, password=raw_password)  # 사용자 인증
            login(request, user)  # 로그인
            #DuoUniversalAuthMiddleware(settings.DUO_UNIVERSAL_AUTH, request):
            return redirect('index')
    else:
        form = UserForm()
    return render(request, 'login/signup.html', {'form': form})

class PasswordResetView(auth_views.PasswordResetView):
    """
    비밀번호 초기화 - 사용자ID, email 입력
    """
    template_name = 'login/password_reset.html'
    success_url = reverse_lazy('login:password_reset_done')
    form_class = PasswordResetForm
    # email_template_name = 'common/password_reset_email.html'


class PasswordResetDoneView(auth_views.PasswordResetDoneView):
    """
    비밀번호 초기화 - 메일 전송 완료
    """
    template_name = 'login/password_reset_done.html'


class PasswordResetConfirmView(auth_views.PasswordResetConfirmView):
    """
    비밀번호 초기화 - 새로운 비밀번호 입력
    """
    template_name = 'login/password_reset_confirm.html'
    success_url = reverse_lazy('login:login')

def LoginInfo(request):
    form = FakeUserForm()
    return render(request, 'login/logininfo.html', {'form': form})
#class LoginInfoView():
#    template_name = 'login/password_reset_confirm.html'
#    success_url = reverse_lazy('login:login')
