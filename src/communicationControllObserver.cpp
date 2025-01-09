//  Copyright (c) 2025 Grigoriy Efimov
// 
// Licensed under the MIT License. See LICENSE file for details.

#include "headers/communicationControllObserver.hpp"
#include "headers/mouseControllObserver.hpp"
#include "headers/server.hpp"
#include <string>

#define COMMUNICATION_HELLO "HELLO;SUPPORT:v1"
#define COMMUNICATION_REJECT_SETUP "REJECT;SUPPORT:v1"
#define COMMUNICATION_V1_SETUP_FAIL "SETUP_FAIL"
#define COMMUNICATION_ALLREADY_AUTHORIZED "ALLREADY_AUTHORIZATION"
#define COMMUNICATION_AUTHORIZATION_SUCCESS "AUTHORIZATION_SUCCESS"
#define COMMUNICATION_AUTHORIZATION_FAILED "AUTHORIZATION_FAILED"
#define COMMUNICATION_SETUP_PREFIX "SETUP:"
#define COMMUNICATION_SETUP_SUCCESS "SETUP_SUCCESS"
#define COMMUNICATION_AUTHORIZATION_REQUIRED "AUTHORIZATION_REQUIRED"
#define COMMUNICATION_AUTHORIZATION_WAITING "AUTHORIZATION_WAITING"

#define COMMUNICATION_REQUEST_AUTHORIZATION "AUTH_REQUEST"

namespace remoteMouse {

    CommunicationControllObserver::CommunicationControllObserver(std::unique_ptr<AuthorizationController> authController, bool skipAuth, std::string deviceName){
        if(skipAuth){
            isAuthorized = true;
        }
        this->skipAuth = skipAuth;
        this->authController = std::move(authController);
        this->deviceName = deviceName;
    }

    void CommunicationControllObserver::notifyConnect([[maybe_unused]] sockaddr_in adder, Server* server, int client_fd){
        server->answer(client_fd, COMMUNICATION_HELLO);
    }
    void CommunicationControllObserver::notifyDisconnect([[maybe_unused]] sockaddr_in adder, [[maybe_unused]] Server*){
        if(!skipAuth){
            isAuthorized = false;
        }
    }
    void CommunicationControllObserver::notifyNewMessage(const char* message, Server* server, int client_fd){
        std::string msg = {message};
        msg = trimNewline(msg);
        if(msg == COMMUNICATION_REQUEST_AUTHORIZATION){
            if(isAuthorized){
                server->answer(client_fd, COMMUNICATION_ALLREADY_AUTHORIZED);
            }
            else{
                server->answer(client_fd, COMMUNICATION_AUTHORIZATION_WAITING);
                if(authController->authorize()){
                    isAuthorized = true;
                    server->answer(client_fd, COMMUNICATION_AUTHORIZATION_SUCCESS);
                }
                else{
                    isAuthorized = false;
                    server->answer(client_fd, COMMUNICATION_AUTHORIZATION_FAILED);
                }
            }
        }
        else if(msg.find(COMMUNICATION_SETUP_PREFIX) == 0){
            if(isAuthorized){
                std::string setup = msg.substr(strlen(COMMUNICATION_SETUP_PREFIX));
                if(setup.find("v1") == 0){
                    setup = setup.substr(strlen("v1"));
                    if(setup == "rel"){
                        server->addObserver(std::make_unique<MouseControllObserver>(deviceName));
                        server->answer(client_fd, COMMUNICATION_SETUP_SUCCESS);
                    }
                    else{
                        server->answer(client_fd, COMMUNICATION_V1_SETUP_FAIL);
                    }
                }
                else{
                    server->answer(client_fd, COMMUNICATION_REJECT_SETUP);
                }
            }
            else{
                server->answer(client_fd, COMMUNICATION_AUTHORIZATION_REQUIRED);
            }
        }
        else{
            if(!isAuthorized){
                server->answer(client_fd, COMMUNICATION_AUTHORIZATION_REQUIRED);
            }
        }
    }

    std::string CommunicationControllObserver::trimNewline(std::string str) {
        size_t pos = str.find_last_not_of("\n\r");
        if (pos != std::string::npos) {
            str.erase(pos + 1);
        }
        return str;
    }

}