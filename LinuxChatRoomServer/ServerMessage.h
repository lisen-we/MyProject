#pragma once
bool SendCreateUserResponse(int clientFd, bool registered);
bool SendUserLoginResponse(int clientFd, bool loggedIn);