import sys
import socket
from threading import Thread

def server(port):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("", int(port)))
    server_socket.listen(1)
    print(f"Listening on port {port}")

    client_socket, client_address = server_socket.accept()
    print(f"Connection from {client_address}")

    def receive():
        while True:
            message = client_socket.recv(1024).decode("utf-8")
            if not message:
                break
            print(f"Client: {message}")

    def send():
        while True:
            message = input()
            if message.lower() == "quit":
                client_socket.close()
                break
            client_socket.send(message.encode("utf-8"))

    receive_thread = Thread(target=receive)
    send_thread = Thread(target=send)

    receive_thread.start()
    send_thread.start()

    receive_thread.join()
    send_thread.join()

def client(ip, port):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((ip, int(port)))

    def receive():
        while True:
            message = client_socket.recv(1024).decode("utf-8")
            if not message:
                break
            print(f"Server: {message}")

    def send():
        while True:
            message = input()
            if message.lower() == "quit":
                print("Closing connection...");
                client_socket.close()
                break
            client_socket.send(message.encode("utf-8"))

    receive_thread = Thread(target=receive)
    send_thread = Thread(target=send)

    receive_thread.start()
    send_thread.start()

    receive_thread.join()
    send_thread.join()

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage:")
        print("  Server: stnc -s PORT")
        print("  Client: stnc -c IP PORT")
        sys.exit(1)

    if sys.argv[1] == "-s":
        server(sys.argv[2])
    elif sys.argv[1] == "-c":
        client(sys.argv[2], sys.argv[3])
    else:
        print("Invalid option.")
        sys.exit(1)
