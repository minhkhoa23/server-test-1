#include "stdafx.h"
#include "afxsock.h"
#include <iostream>
#include <csignal>
#include <vector>
#include <fstream>
#include <cstring>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

CWinApp theApp;
bool running = true;

#define BUFFER_SIZE 1024

void signalHandler(int signum) {
    cout << "Tin hieu ngat (" << signum << ") nhan duoc. Dang tat may chu...\n";
    running = false;
}

// Cau truc de luu thong tin ve file
struct FileInfo {
    string name;
    string size;
};

// Ham doc danh sach file tu file van ban
vector<FileInfo> readFileList(const char* fileName) {
    ifstream file(fileName);
    vector<FileInfo> fileList;

    if (!file.is_open()) {
        cerr << "Loi mo danh sach file: " << fileName << endl;
        return fileList;
    }

    string name, size;
    while (file >> name >> size) {
        fileList.push_back({ name, size });
    }

    file.close();
    return fileList;
}

// Ham gui danh sach file cho client
void sendFileList(CSocket& clientSocket, const vector<FileInfo>& fileList) {
    string fileListString;
    for (const auto& fileInfo : fileList) {
        fileListString += fileInfo.name + " " + fileInfo.size + "\n";
    }

    clientSocket.Send(fileListString.c_str(), fileListString.size());
}

// Ham gui file cho client
void sendFile(CSocket& clientSocket, const char* fileName) {
    ifstream file(fileName, ios::binary);
    if (!file.is_open()) {
        cerr << "Loi mo file: " << fileName << endl;
        clientSocket.Send("ERROR", 5); // Gui thong bao loi cho client neu file khong tim thay
        return;
    }

    clientSocket.Send("GREAT", 5);

    file.seekg(0, ios::end);
    long long fileSize = file.tellg();
    file.seekg(0, ios::beg);

    clientSocket.Send(&fileSize, sizeof(fileSize));

    char buffer[BUFFER_SIZE];
    while (!file.eof()) {
        file.read(buffer, BUFFER_SIZE);
        int bytesRead = file.gcount();
        clientSocket.Send(buffer, bytesRead);
    }

    file.close();
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[]) {
    signal(SIGINT, signalHandler);
    int nRetCode = 0;

    if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) {
        _tprintf(_T("Loi nghiem trong: Khoi tao MFC that bai\n"));
        nRetCode = 1;
    }
    else {
        if (AfxSocketInit() == FALSE) {
            cout << "Khong the khoi tao thu vien socket" << endl;
            return 1;
        }

        vector<FileInfo> fileList = readFileList("FileList.txt");

        CSocket serverSocket;
        if (serverSocket.Create(1234) == 0) {
            cout << "Khong the khoi tao may chu " << endl;
            cout << serverSocket.GetLastError() << endl;
            return 1;
        }
        else {
            cout << "Khoi tao may chu thanh cong !" << endl;

            if (serverSocket.Listen(5) == FALSE) {
                cout << "Khong the lang nghe tren cong nay " << endl;
                serverSocket.Close();
                return 1;
            }
           
        }

        while (running) {
            CSocket clientSocket;
            if (serverSocket.Accept(clientSocket)) {
                cout << "Ket noi client thanh cong " << endl;

                sendFileList(clientSocket, fileList);

                char filename[BUFFER_SIZE];
                while (true) {
                    memset(filename, 0, BUFFER_SIZE); // Lam sach buffer truoc khi nhan du lieu
                    int bytesReceived = clientSocket.Receive(filename, BUFFER_SIZE - 1);
                    if (bytesReceived <= 0) {
                        break;
                    }; // Ket thuc neu khong nhan duoc du lieu

                    filename[bytesReceived] = '\0'; // Dam bao chuoi ky tu ket thuc dung cach

                    cout << "Client yeu cau tai file: " << filename << endl;

                    // Kiem tra xem tap tin co ton tai trong danh sach file hay khong
                    bool fileFound = false;
                    for (const auto& fileInfo : fileList) {
                        if (fileInfo.name == filename) {
                            fileFound = true;
                            break;
                        }
                    }

                    if (fileFound) {
                        sendFile(clientSocket, filename);
                    }
                    else {
                        cerr << "File khong tim thay trong danh sach: " << filename << endl;
                        clientSocket.Send("ERROR", 5);
                    }
                }

                clientSocket.Close();
                cout << "File da duoc gui, dong ket noi" << endl;
            }
            else {
                cout << "Ket noi client khong thanh cong" << endl;
            }
        }

        serverSocket.Close();
    }
    return nRetCode;
}
