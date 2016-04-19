// Test cases related to shell command
var shell = {
    /* 0.0.431-8fb76c5 */
    version : "\\d+\\.\\d+.\\d+-[a-z0-9]+",
    /* Enabled | Disabled | Not Supported */
    turbo   : "(enabled|disabled|not support)",
    /* Hello */
    echo    : "hello",
    /* Thu Apr 14 11:19:45 UTC 2016 */
    date    : "(Sun|Mon|Tue|Wed|Thu|Fri|Sat)\\s+(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\\s+\\d{2}\\s+\\d{2}:\\d{2}:\\d{2}\\s+UTC\\s+\\d{4}",
    manager : {
        /* 192.168.100.254 */
        ip      : "\\d{3}\\.\\d{3}.\\d{3}.\\d{3}",
        /* 111 */
        port    : "\\d{1,4}",
        /* 192.168.100.200 */
        gateway : "\\d{3}\\.\\d{3}.\\d{3}.\\d{3}",
    },
}

// Test which executed in kernel. 
var kernel = {
    test : {
        /* 
         * Test case report format 
         *
         * [ Test 001 ] test_open
         * [ PASS     ]
         * [==========]
         * [ Test 002 ] test_close
         * [     FAIL ] ../src/test/file.c:29: file_close should return 0 for success
         */
        name        : "\\[ Test \\d+ \\] (.*)",
        pass        : "\\[ PASS\\s+\\]",
        fail        : "\\[\\s+FAIL \\] (.*)",
        /* 
         * Final report string
         *
         * Test Cases: 4  Suceess: 3  Errors: 1 
         */
        cases       : "Test Cases: (\\d*)",
        success     : "Success: (\\d*)",
        errors      : "Errors: (\\d*)",
    }
}

exports.shell = shell;
exports.kernel = kernel;
