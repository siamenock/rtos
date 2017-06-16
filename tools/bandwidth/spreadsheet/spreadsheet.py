from __future__ import print_function

import httplib2
import os

from googleapiclient.errors import HttpError

from parser import FileDataParser, argparser

from apiclient import discovery
from oauth2client import client
from oauth2client import tools
from oauth2client.file import Storage

import datetime

try:
    import argparse
    flags = argparse.ArgumentParser(parents=[argparser]).parse_args()
except ImportError:
    flags = None

# If modifying these scopes, delete your previously saved credentials
# at ~/.credentials/sheets.googleapis.shreadsheet.json
SCOPES = [ 'https://www.googleapis.com/auth/spreadsheets',\
         'https://www.googleapis.com/auth/drive' ]

CLIENT_SECRET_FILE = 'client_secret.json'
APPLICATION_NAME = 'Google Sheets API Python Quickstart'

def getCredentials():
    """Gets valid user credentials from storage.

    If nothing has been stored, or if the stored credentials are invalid,
    the OAuth2 flow is completed to obtain the new credentials.

    Returns:
    Credentials, the obtained credential.
    """
    home_dir = os.path.expanduser('~')
    credential_dir = os.path.join(home_dir, '.credentials')
    if not os.path.exists(credential_dir):
        os.makedirs(credential_dir)
    credential_path = os.path.join(credential_dir,
                                   'sheets.googleapis.spreadsheet.json')

    store = Storage(credential_path)
    credentials = store.get()
    if not credentials or credentials.invalid:
        flow = client.flow_from_clientsecrets(CLIENT_SECRET_FILE, SCOPES)
        flow.user_agent = APPLICATION_NAME
        if flags:
            credentials = tools.run_flow(flow, store, flags)
        else:  # Needed only for compatibility with Python 2.6
            credentials = tools.run(flow, store)

        print('Storing credentials to ' + credential_path)
    return credentials

class FileService:
    FILE_TYPE = 'application/vnd.google-apps.spreadsheet'

    def __init__(self, credential):
        http = credential.authorize(httplib2.Http())
        self.service = discovery.build('drive', 'v3', http=http)

    # NOTE: do not use this API directly. Create spreadsheet by
    # SpreadsheetService
    def create(self, name):
        data = {
            'name': '%s' % name,
            'mimeType': '%s' % FileService.FILE_TYPE,
        }
        file = self.service.files().create(body=data).execute()
        print("File '%s' (%s) created" % (file['name'], file['id']))

        return file

    def get(self, name):
        token = None
        while True:
            query = "name='%s' and mimeType='%s' and trashed=False" % (name,
                    FileService.FILE_TYPE)
            response = self.service.files().list(q=query, spaces='drive',
                    fields='nextPageToken, files(id, name)', pageToken=token).execute()

            for file in response.get('files', []):
                # Return matched file for the first time
                return file

            token = response.get('nextPageToken', None)
            if token is None:
                break

        return None

    def delete(self, name):
        token = None
        while True:
            query = "name='%s' and mimeType='%s' and trashed=False" % (name,
                    FileService.FILE_TYPE)
            response = self.service.files().list(q=query, spaces='drive',
                    fields='nextPageToken, files(id, name)',
                    pageToken=token).execute()

            for file in response.get('files', []):
                self.service.files().delete(fileId=file['id']).execute()
                print("File '%s' (%s) deleted" % (file['name'], file['id']))

            token = response.get('nextPageToken', None)
            if token is None:
                break

class Spreadsheet:
    def __init__(self, spreadsheetId, spreadsheet):
        self.spreadsheetId = spreadsheetId
        self.spreadsheet = spreadsheet
        self.properties = spreadsheet['properties']
        self.title = self.properties['title']

class Worksheet:
    def __init__(self, worksheetId, properties, charts=None):
        self.worksheetId = worksheetId
        self.properties = properties
        self.title = properties['title']
        self.charts = charts

class SpreadsheetService:
    def __init__(self, credential):
        http = credential.authorize(httplib2.Http())
        url = "https://sheets.googleapis.com/$discovery/rest?version=v4"
        self.service = discovery.build('sheets', 'v4', http=http,
                              discoveryServiceUrl=url)
        self.spreadsheet = None
        self.worksheets = {}

    def registerSpreadsheet(self, spreadsheet):
        # Spreadsheet
        self.spreadsheet = Spreadsheet(spreadsheet['spreadsheetId'], spreadsheet)
        # Default worksheet
        worksheet = spreadsheet['sheets'][0]
        worksheetId = worksheet['properties']['sheetId']
        worksheetProperties = worksheet['properties']
        if 'charts' in worksheet:
            worksheetCharts = worksheet['charts']
        else:
            worksheetCharts = None

        self.worksheets[worksheetId] = Worksheet(worksheetId, worksheetProperties,
                                                 worksheetCharts)

    def createSpreadsheet(self, name):
        data = {
            "properties": {
                "title": "%s" % name,
            },
            "sheets": {
                "properties": {
                    "title": "Summary"
                }
            }
        }
        spreadsheet = self.service.spreadsheets().create(body=data).execute()
        self.registerSpreadsheet(spreadsheet)

        print("Spreadsheet '%s' (%s) created" % (self.spreadsheet.title,
                                                 self.spreadsheet.spreadsheetId))

        return self.spreadsheet

    def getSpreadsheet(self, file):
        spreadsheet = self.service.spreadsheets().get(spreadsheetId=file['id']).execute()

        self.registerSpreadsheet(spreadsheet)

        print("Spreadsheet '%s' (%s) found" % (self.spreadsheet.title,
                                               self.spreadsheet.spreadsheetId))

        return self.spreadsheet

    def addWorksheet(self, name):
        data = {
            "requests": [
                {
                    "addSheet": {
                        "properties": {
                            "title": "%s" % name
                        }
                    }
                }
            ]
        }
        try:
            result = self.service.spreadsheets().batchUpdate(
                spreadsheetId=self.spreadsheet.spreadsheetId, body=data).execute()
        # WARN: cannot add a worksheet which has existed name
        except HttpError:
            print("Invalid request. Failed to make worksheet '%s'" % name)
            return None

        response = result['replies'][0]

        sheetProperties = response['addSheet']['properties']
        worksheetId = sheetProperties['sheetId']
        worksheet = Worksheet(worksheetId, sheetProperties)

        self.worksheets[worksheetId] = worksheet

        print("Worksheet '%s' (%d) added" % (worksheet.title, worksheet.worksheetId))
        return worksheet

    def deleteWorksheet(self, worksheetId):
        title = self.worksheets[worksheetId]
        if title is None:
            print("Failed to find worksheet (%s)" % worksheetId)
            return

        data = {
            "requests": [
                {
                    "deleteSheet": {
                        "sheetId": "%s" % worksheetId
                    }
                }
            ]
        }
        self.service.spreadsheets() \
            .batchUpdate(spreadsheetId=self.spreadsheet.spreadsheetId, body=data).execute()

        self.worksheets.pop(worksheetId)

        print("Worksheet '%s' (%s) deleted" % (title, worksheetId))

    def getWorksheet(self, title):
        for worksheetId, worksheet in self.worksheets.items():
            if title in worksheet.title:
                return worksheet

    def record(self, worksheetId, file, mode):
        worksheet = self.worksheets[worksheetId]
        if worksheet is None:
            print("Failed to find worksheet (%s)" % worksheetId)
            return

        title = worksheet.title
        body = {
            'values': FileDataParser(file, mode).parseData(),
        }

        self.service.spreadsheets().values().update(
            spreadsheetId=self.spreadsheet.spreadsheetId,
            valueInputOption="USER_ENTERED", range=title,
            body=body).execute()

    def dimension(self, worksheet):
        title = worksheet.title

        valueRange = self.service.spreadsheets().values().get(
            spreadsheetId=self.spreadsheet.spreadsheetId, range=title).execute()

        row = 0
        column = 0
        if 'values' in valueRange.keys():
            row = len(valueRange['values'])
            column = len(valueRange['values'][0])
        return (row, column)

    def summary(self, worksheet, files, mode, category=None, option=None):
        values = []
        range = worksheet.title

        # Check whether this file data is added for the first time in the worksheet
        (endRowIndex, endColumnIndex) = self.dimension(worksheet)
        if endColumnIndex == 0:
            axis = []
            if 'Throughput' in mode:
                # Insert byte size value to left axis
                for file in files:
                    parser = FileDataParser(file, mode)
                    axis.append(parser.parseCommand('byte'))
            elif 'Latency' in mode:
                # Insert any option to left axis
                if option is not None:
                    axis.append(option)

            axis.insert(0, mode)
            values.append(axis)
        else:
            row = endColumnIndex
            range += "!%c1" % chr(row + 97)

        # Insert result values
        results = []
        for file in files:
            parser = FileDataParser(file, mode)
            results.append(parser.parseResult())

        results.insert(0, category)
        values.append(results)

        body = {
            'majorDimension': 'COLUMNS',
            'values': values
        }

        self.service.spreadsheets().values().update(
            spreadsheetId=self.spreadsheet.spreadsheetId,
            valueInputOption="USER_ENTERED", range=range, body=body).execute()

    def getChart(self, worksheet, title):
        for chart in worksheet.charts:
            if title in chart['spec']['title']:
                print("Chart %s found"% title)
                return chart

        return None

    def deleteChart(self, worksheet, chart):
        data = {
            "requests": [
                {
                    "deleteEmbeddedObject": {
                        "objectId": chart['chartId']
                    }
                }
            ]
        }
        self.service.spreadsheets().batchUpdate(spreadsheetId=self.spreadsheet.spreadsheetId,
                                                body=data).execute()
        print("Chart (%s) deleted in worksheet '%s'" % (chart['chartId'], worksheet.title))

    def drawLineChart(self, worksheet):
        pass

    def drawChart(self, worksheet, mode):
        (endRowIndex, endColumnIndex) = self.dimension(worksheet)

        if 'Throughput' in mode:
            chartType = "LINE"
        elif 'Latency' in mode:
            chartType = "COLUMN"
        else:
            print("Mode '%s' unsupported" % mode)
            return

        data = {
            "requests": [
                {
                    "addChart": {
                        "chart": {
                            "spec": {
                                "title": mode,
                                "basicChart": {
                                    "chartType": chartType,
                                    "legendPosition": "BOTTOM_LEGEND",
                                    "axis": [
                                        {
                                            "position": "LEFT_AXIS",
                                            "title": "Byte Per Second"
                                        }
                                    ],
                                    # Left axis
                                    "domains": [
                                    ],
                                    # Series of result data
                                    "series": [
                                    ],
                                    "headerCount": 1
                                }
                            },
                            "position": {
                                "overlayPosition": {
                                    "anchorCell": {
                                        "sheetId": worksheet.worksheetId,
                                        "rowIndex": 10,
                                        "columnIndex": 1
                                    },
                                }
                            }
                        }
                    }
                }
            ]
        }

        domains = data['requests'][0]['addChart']['chart']['spec']['basicChart']['domains']
        domains.append({
            "domain": {
                "sourceRange": {
                    "sources": [
                        {
                            "sheetId": worksheet.worksheetId,
                            "startRowIndex": 0,
                            "endRowIndex": endRowIndex,
                            "startColumnIndex": 0,
                            "endColumnIndex": 1
                        }
                    ]
                }
            }
        })

        # Insert data column by column
        series = data['requests'][0]['addChart']['chart']['spec']['basicChart']['series']
        for index in range(1, endRowIndex - 1):
            series.append({
                "series": {
                    "sourceRange": {
                        "sources": [
                            {
                                "sheetId": worksheet.worksheetId,
                                "startRowIndex": 0,
                                "endRowIndex": endRowIndex,
                                "startColumnIndex": index,
                                "endColumnIndex": index + 1
                            }
                        ]
                    }
                },
                "targetAxis": "LEFT_AXIS"
            })

        self.service.spreadsheets().batchUpdate(spreadsheetId=self.spreadsheet.spreadsheetId,
                                                         body=data).execute()
        print("Chart added to '%s' worksheet" % worksheet.title)

def main():
    # Default arguments
    category = 'PacketNgin'
    mode = 'Throughput'
    name = 'PacketNgin Throughput'

    if flags.name is not None:
        name = flags.name

    if flags.latency is True:
        mode = 'Latency'
        name.replace('Throughput', mode)

    if flags.category is not None:
        category = flags.category

    # Default file name is like this "2017-01-10 PacketNgin Throughput"
    name = datetime.date.today().isoformat() + ' ' + name
    credentials = getCredentials()

    # Create or get spreadsheet
    service = SpreadsheetService(credentials)

    file = FileService(credentials).get(name)
    if file is not None:
        print(file)
        service.getSpreadsheet(file)
    else:
        service.createSpreadsheet(name)

    worksheet = service.getWorksheet('Summary')

    # Remove existed charts
    if worksheet.charts is not None:
        for chart in worksheet.charts:
            service.deleteChart(worksheet, chart)

    # Prepare all of pages which sorted by option in a spreadsheet
    files = flags.file
    pages = []
    for file in files:
        if 'Throughput' in mode:
            pages.append([FileDataParser(file, mode).parseCommand('byte'), file])
        else:
            pages.append(['1', file])

    pages.sort(key=lambda x: int(x[0]))

    # Add worksheet
    worksheets = []
    files = []
    for page in pages:
        worksheet = service.addWorksheet('%s_%s' % (category, page[0]))
        if worksheet is None:
            continue
        worksheets.append(worksheet)
        # Record result
        service.record(worksheet.worksheetId, page[1], mode)
        # Prepare sorted files
        files.append(page[1])

    if files:
        worksheet = service.getWorksheet('Summary')
        service.summary(worksheet, files, mode, category)
        service.drawChart(worksheet, mode)

if __name__ == '__main__':
    main()
