import sys

def parse_line(line):
    len = ""
    msg = ""
    for i, c in enumerate(line):
        if c.isdigit():
            len += c
        elif i > 0 and c != ' ' and c != '\n':
            msg += c

    return (len, msg)


def verify_echo_test():
    s = []
    r = []
    for line in sys.stdin:
        res = parse_line(line)
        if(line[0] == 's'):
            s.append(res)
        else:
            r.append(res)

    if(len(s) != len(r)):
        return False

    for i in range(0, len(r)):
        if(s[i][0] != r[i][0]):
            return False
        if(s[i][1] != r[i][1]):
            return False

    return True


def main():
    print("YES" if verify_echo_test() else "NO")
if __name__ == '__main__':
    main()
