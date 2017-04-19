## Spreadsheet Utility

#### Description
Automatic recoder which enables user to add their file data to Google spreadsheet. Several file data can be written in a spreadsheet by several worksheets, summarized by predefined result data and drawn in the shape of chart. 

We used python goolge API client.

#### Prerequisites
Complete Step 1 and Step 2 in [Google Sheet APIv4 reference](https://developers.google.com/sheets/api/quickstart/python). You should have **client_secret.json** file in the project folder.

#### Usage
```shell
$ python ./spreadsheet.py FILES MODE [CATEGORY] [NAME]
```
###### Options
> FILES (required)
>> Name of files which have data and to be inserted.

> MODE (required)
>> Testing mode. 
>>> **-l** : Latency mode <br>
>>> **-t** : Throughput mode

> CATEGORY
>> Category of data. Can be categorized by any string.
>>> **-c PacketNgin** <br>
>>> **-c Linux** 

> NAME
>> Name of spreadsheet will be created. Current date is automatically appended to head.
>>> **-n "PacketNgin Througput Test"**<br>
>>> **-n "Linux Latency Test"**

#### Contact
GurumNetworks <http://gurum.cc><br>
Developed by <youseok@gurum.cc>

#### License
MIT
