import socket
import struct
import random
import sys
import os

class Conn:
    def __init__(self, sock):
        self.sock = sock
        self.read_buffer = bytearray()
        self.read_buffer_size = 0

    def process_one_message(self):
        if self.read_buffer_size < HEADER_SIZE:
            return False

        msg_len = struct.unpack("!I", self.read_buffer[:4])[0]

        if self.read_buffer_size < msg_len + 4:
            return False

        msg = self.read_buffer[4: 4 + msg_len].decode('utf-8')
        print(f'received len {len(msg)} msg: {msg}\n')
        #f.write(f'r {msg_len} {msg}\n')

        self.cut_message(msg_len)
        #memmove(buffer, buffer + (4 + msg_len), size - (4 + msg_len))
        # TODO: cut message


        return True
    def cut_message(self, msg_len):
        r = (self.read_buffer_size) - (4 + msg_len)
        if r > 0:
            #if self.read_buffer_size - (4 + msg_len) > 0:
            start = 4 + msg_len
            end = max((start + r), MAX_CAP)

            print("start: %d" % start)
            print("end: %d" % end)

            assert(start < end)
            print(len(self.read_buffer))
            tmp = self.read_buffer[start:end]
            self.read_buffer = tmp
            print("len : %d", len(self.read_buffer))
        self.read_buffer_size = r
        #if len(self.read_buffer) == 0:
        #    print("ZERO!")
        #    print("r %d" % r)
        #    return

    def read(self, data, nread):
        #memcpy(buffer + size, data, n)
        data_offset = 0
        while nread > 0:
            #CAP, NREADI
            n = min(MAX_CAP - self.read_buffer_size, nread)
            if n > 0:
                self.read_buffer[self.read_buffer_size:] = data[data_offset:n]

            nread -= n
            self.read_buffer_size += n
            assert self.read_buffer_size <= MAX_CAP
            data_offset += n

            while self.process_one_message():
                pass

#This client sends several pipelined requests of random length and random content
#
MAX_MSG_SIZE = 16 * 1024
HEADER_SIZE = 4
MAX_CAP = MAX_MSG_SIZE + HEADER_SIZE
table = "abcdefgh"
save_filename = os.path.abspath(".") + "/scripts/echo_test.txt"

def rand_str(n):
    ans = ""
    for i in range(0, n):
        ans += table[random.randint(0, len(table) - 1)]
    return ans

def send_pipelined_requests(sock, f, num_request):
    total = 0
    for i in range(0, num_request):
        msg = rand_str(random.randrange(1, MAX_MSG_SIZE -1)).encode('utf-8')
        msg_len = struct.pack("!I", len(msg))
        s = msg.decode('utf-8')

        #print(f'len {len(msg)}\n')
        f.write(f's {len(msg)} {s}\n')

        buffer = msg_len + msg
        total += len(msg) + 4
        sent = 0
        sock.send(buffer)

    return total

def read(sock):
    data = sock.recv(1024 * 64)

def process_many_responses():
    pass

    

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

    total = send_pipelined_requests(sock, f, num_request)
    print("total sent %s" % total)

    conn = Conn(sock)
    while True:
        data = sock.recv(64 * 1024)
        conn.read(data, len(data))
    #while count < num_request:
    #    data = sock.recv((MAX_MSG_SIZE + 4) - received)
    #    if data:
    #        recv_buffer += data
    #        received += len(data)

    #        while True and count < num_request:
    #            if received < HEADER_SIZE:
    #                break

    #            msg_len = struct.unpack("!I", recv_buffer[:4])[0]

    #            if msg_len + 4 > received:
    #                break

    #            msg = (recv_buffer[4:msg_len + 4]).decode('utf-8')



    #            # clean msg
    #            recv_buffer = recv_buffer[4 + msg_len:]
    #            received -= (4 + msg_len)
    #            count += 1

    print("results saved at %s" % save_filename)
    f.close()

if __name__ == '__main__':
    #test = "   sadas kshajd ";
    #print(test.strip())
    main()
