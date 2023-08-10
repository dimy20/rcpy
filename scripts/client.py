import socket
import struct
import random
import sys
import os

#This client sends several pipelined requests of random length and random content
#

MAX_MSG_SIZE = 4096
HEADER_SIZE = 4
table = "abcdefgh"
save_filename = os.path.abspath(".") + "/scripts/echo_test.txt"

def rand_str(n):
    ans = ""
    for i in range(0, n):
        ans += table[random.randint(0, len(table) - 1)]
    return ans

def send_pipelined_requests(sock, f, num_request):
    for i in range(0, num_request):
        msg = rand_str(random.randrange(1, 50)).encode('utf-8')
        msg_len = struct.pack("!I", len(msg))
        s = msg.decode('utf-8')
        f.write(f's {len(msg)} {s}\n')
        buffer = msg_len + msg
        sock.send(buffer)

def show_usage():
    print("Usage: python3 client.py n")
    print("n = number of requests")

def main():
    if len(sys.argv) < 2:
        print("Not enough arguments")
        show_usage()
        sys.exit(1)

    num_request = 0
    try:
        num_request = int(sys.argv[1])
        if num_request < 0:
            print("Argument must be positive")
            show_usage()
            sys.exit(1)

    except ValueError as e:
        print("Argument must be numerical")
        show_usage()
        sys.exit(1)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(("127.0.0.1", 8080))

    f = open(save_filename, "w")
    if not f:
        print("failed to open %s" % save_filename)

    send_pipelined_requests(sock, f, num_request)

    count = 0
    recv_buffer = bytearray()
    received = 0

    while count < num_request:
        data = sock.recv((MAX_MSG_SIZE + 4) - received)
        if data:
            recv_buffer += data
            received += len(data)

            while True and count < num_request:
                if received < HEADER_SIZE:
                    break

                msg_len = struct.unpack("!I", recv_buffer[:4])[0]

                if msg_len + 4 > received:
                    break

                msg = (recv_buffer[4:msg_len + 4]).decode('utf-8')
                f.write(f'r {msg_len} {msg}\n')

                # clean msg
                recv_buffer = recv_buffer[4 + msg_len:]
                received -= (4 + msg_len)
                count += 1

    print("results saved at %s" % save_filename)
    f.close()

if __name__ == '__main__':
    #test = "   sadas kshajd ";
    #print(test.strip())
    main()
