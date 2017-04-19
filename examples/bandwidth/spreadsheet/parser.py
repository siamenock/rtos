import csv
import collections
from oauth2client import tools

__all__ = ['argparser']

def _CreateArgumentParser():
    try:
        import argparse
    except ImportError:  # pragma: NO COVER
        return None

    # FIXME: process oauth arguments
    parser = argparse.ArgumentParser(parents=[tools.argparser],
                                     description="PacketNgin performance spreadsheet recorder",
                                     add_help=False)
    parser.add_argument('-n', '--name', help="Name of spreadsheet file recorded in")
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('-t', '--throughput', action='store_true', help="Throughput record mode")
    mode.add_argument('-l', '--latency', action='store_true', help="Latency record mode")
    parser.add_argument('-c', '--category',
                        help="Category of data file. PacketNgin or Linux")
    parser.add_argument('file', nargs='+', type=argparse.FileType('r'),
                        help="Data file to be recorded")

    return parser

argparser = _CreateArgumentParser()

# Mode parsers
parsers = {
    'Throughput': {
        # Predefined command format
        'commandFormat': collections.namedtuple('command',
                                               # java -cp bin Client 1 1500 192.168.200.10 7 3
                                               ['command', 'options', 'directory', 'type',
                                                'thread', 'byte', 'ip', 'port', 'count']),
        # Predefined result format
        'resultFormat': collections.namedtuple('result',
                                              # Avg   81,426  0   977,116,000 0
                                              ['average', 'tx_pps', 'rx_pps', 'tx_bps', 'rx_bps']),

        # Predefined delimiter of result
        'delimiter': '=',
        # Variable name which locate result value
        'result': 'rx_bps',
        # Variable name which locate result value
        'result': 'rx_bps',
    },
    'Latency': {
        'commandFormat': None,
        'resultFormat': collections.namedtuple('result',
                                               # rtt min/avg/max/mdev = 0.167/0.529/1.226/0.271 ms
                                               ['rtt', 'kind', 'equal', 'results', 'ms']),

        # 10 packets trasmitted, 10 received, 0% packet loss, time 9000ms
        'delimiter': 'packets',
        'result': 'results',
    },
}

# Data type parser
class FileDataParser:
    def __init__(self, file, mode):
        self.file = file
        self.mode = mode
        parser = parsers[mode]
        self.format = {'command': parser['commandFormat'],
                       'result': parser['resultFormat']}
        self.result = parser['result']
        self.delimiter = parser['delimiter']

    def parseCommand(self, format):
        # Latency mode does not implement command parsing
        assert 'latency' not in self.mode

        self.file.seek(0)
        command = self.file.readline().strip().split(' ')
        commandFormat = self.format['command']._make(command)

        return getattr(commandFormat, format)

    def parseData(self):
        self.file.seek(0)
        data = []
        reader = csv.reader(self.file, dialect='excel-tab', delimiter=' ')
        for line in reader:
            data.append(line)

        return data

    def parseResult(self):
        self.file.seek(0)
        reader = csv.reader(self.file, dialect='excel-tab', delimiter=' ')
        for line in reader:
            if not line:
                continue

            # XXX: not iterate every single cell
            for word in line:
                if self.delimiter in word:
                    line = reader.next()
                    resultFormat = self.format['result']._make(line)
                    result = getattr(resultFormat, self.result)

                    return result

        return None

class MemoryDataParser:
    # Not yet implemented
    pass


