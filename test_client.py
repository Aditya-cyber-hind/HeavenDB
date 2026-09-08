import socket
import time

def connect():
    s = socket.socket()
    s.connect(('localhost', 6379))
    return s

def send_command(s, cmd):
    s.send((cmd + '\n').encode())
    time.sleep(0.5)
    return s.recv(8192).decode()

# Connect
s = connect()
print(s.recv(4096).decode())  # Welcome message

# Run some commands
print("=== SELECT ===")
print(send_command(s, "SELECT * FROM users"))

print("=== CREATE TABLE ===")
print(send_command(s, "CREATE TABLE orders (id INTEGER, user_id INTEGER, total INTEGER)"))

print("=== INSERT ===")
print(send_command(s, "INSERT INTO orders VALUES (1, 1, 500)"))
print(send_command(s, "INSERT INTO orders VALUES (2, 2, 750)"))

print("=== SELECT ===")
print(send_command(s, "SELECT * FROM orders"))

s.send(b'exit\n')
s.close()
print("Done!")