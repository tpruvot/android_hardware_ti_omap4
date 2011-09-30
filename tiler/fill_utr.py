# Use this script to fill out the UTR for MemMgr/D2C tests.  You can use
# either Python 2.6 or 3.1
#
# Limitations:
#   - only basic tests run by memmgr_test and d2c_test are filled out
#   - negative memmgr_tests are performed in one step and this script will
#     not separate which part of the test failed.  If your negative memmgr
#     tests fail, you want to manually check which test case failed.

# Usage:
#  1. run memmgr_test and d2c_test, and pipe the output of the tests into a
#     file.
# 
#     memmgr_test > test.log
#     d2c_test >> test.log
# 
#  2. open UTR and select Tiler/D2C User Space worksheet
#  3. run this script by piping the test logs into the script, e.g.
#     python fill_utr.py < test.log

import sys, re, win32com.client

TCs, results = [], {}

# gather test results from the log
for line in sys.stdin:
    # check for test description
    desc = re.search("TEST #... - (.*)", line.strip())
    if desc:
        TCs.append(desc.group(1))

    # check for any test result
    result = re.search("==> TEST (.*)", line.strip())
    if result:
        result = result.group(1)
        if result == 'OK':
            result = 'P', ''
        elif result == 'NOT AVAILABLE':
            result = 'D', ''
        else:
            result = 'F', 'failed with error #' + result[5:-1]

        results[TCs.pop(0)] = result

assert "test_d2c(1920, 1080, 2)" in results

# populate test results
excel = win32com.client.Dispatch('Excel.Application')
if not excel:
    print('Could not find Excel application.  The UTR is probably not open in Excel.')
    sys.exit(1)

sheet = excel.ActiveSheet
if sheet.Name != 'Tiler' or sheet.Cells(1,1).Text != 'Test Metrics':
    print('Active worksheet does not seem to be the Tiler UTR')
    sys.exit(1)

# find Header
for r in range(1, 100):
    if sheet.Cells(r, 1).Text == 'Test Case\nID':
        break
else:
    print('Could not find header row starting with "Test Case\\nID"')
    sys.exit(1)

# find Status, Details and Comments columns
heads = t_status, t_details, t_comments, t_desc = 'Status', 'Details', 'Comments', 'Test Case\nDescription'
heads = set(heads)
cols = {}
for c in range(1, 100):
    if sheet.Cells(r, c).Text in heads:
        cols[sheet.Cells(r, c).Text] = c
        heads.remove(sheet.Cells(r, c).Text)
if heads:
    print('Could not find columns', heads)
    sys.exit(1)

# populate sheet, quit after 20 empty Details/Description cells
empties = 0
while empties < 20:
    r += 1
    details = sheet.Cells(r, cols[t_details]).Text
    if details:
        empties = 0
        # get test case description
        desc = details.split('\n')[-1]
        if not desc in results:
            results[desc] = 'U', ''
        sheet.Cells(r, cols[t_status]).Value, sheet.Cells(r, cols[t_comments]).Value = results[desc]
    elif not sheet.Cells(r, cols[t_desc]).Text:
        empties += 1

