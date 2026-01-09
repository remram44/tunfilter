import codecs
import sys

chars = set(b'0123456789ABCDEFabcdef')

def main():
    if sys.argv[1] == 'enc':
        while True:
            ret = sys.stdin.buffer.read(1)
            if not ret:
                return
            sys.stdout.buffer.write(codecs.encode(ret, 'hex_codec'))
            sys.stdout.buffer.flush()
            #sys.stderr.write('>')
            #sys.stderr.flush()
    elif sys.argv[1] == 'dec':
        buf = b''
        while True:
            ret = sys.stdin.buffer.read(2 - len(buf))
            if not ret:
                return
            ret = bytes(c for c in ret if c in chars)
            buf += ret
            if len(buf) == 2:
                sys.stdout.buffer.write(codecs.decode(buf, 'hex_codec'))
                sys.stdout.buffer.flush()
                buf = b''
                #sys.stderr.write('<')
                #sys.stderr.flush()
    else:
        assert False


if __name__ == '__main__':
    main()
