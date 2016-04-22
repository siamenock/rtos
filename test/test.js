// Test cases set
var cases = require('./case.js');
var jUnitWriter = require('junitwriter');
var writer = new jUnitWriter();

// Write commands of test cases to PacketNgin shell file
var fs = require('fs');

if(process.argv.length < 4) {
    consoole.log("Usage : node test.js [fifo file] [result file]");
    process.exit(-1);
}

var fifoFile = process.argv[2];
var resultFile = process.argv[3];

// "In" and "out" sematics of fifo file are relative to the PacketNgin console
var fifoInput = fifoFile + '.out';
var fifoOutput = fifoFile + '.in';

console.log(fifoInput, fifoOutput, resultFile);

// Remove old file 
if(fs.existsSync(resultFile))
    fs.unlinkSync(resultFile);

function writeCommand(exp, cmd) {
    if(typeof(exp[cmd]) != 'string') {
        for(var params in exp[cmd]) {
            fs.appendFileSync(fifoOutput, cmd + ' ');
            writeCommand(exp[cmd], params);
        }
    } else {
        fs.appendFileSync(fifoOutput, cmd + '\n');
    }
}

for(var command in cases.shell) {
    writeCommand(cases.shell, command);
}
// Append kernel runtime test command 
fs.appendFileSync(fifoOutput, 'test\n');
// Append shutdown command to end of the commands 
fs.appendFileSync(fifoOutput, 'shutdown\n');

// Readline interface
var readStream = fs.createReadStream(fifoInput);
var readline = require('readline');
var read = readline.createInterface({
      input: readStream,
      output: process.stdout,
      terminal: false
});
// Command string list
function Command(list) {
    // Index of current command
    this.index = 0;
    // Command list which has all commands  
    this.list = list;
}

// Current command 
Command.prototype.current = function() {
    return this.list[this.index];
}

// Move current pointer and return next command
Command.prototype.next = function() {
    return this.list[++this.index];
}

// Parse status flag
var started = false;
// Output string array 
var output = [];

console.log("Test server application started...\n");

var command;
read.on('line', function(line) {
    if(line.indexOf('#') >= 0) {
        // Start parsing from # keyword found
        if(started == false)
            started = true;

        if(output.length != 0) {
            output = output.join('\n');
            test(command, output);
            output = [];
        }

        var index = parseInt(line.search(/[A-Za-z]/));
        if(index > 0) 
            command = new Command(line.substring(index).split(" "));
        
        // End parsing by shutdown command 
        if(command.current() == 'shutdown') {
            // Generate XML format result
            fs.appendFileSync(resultFile, writer.toString());
            // Close readStream
            readStream.close();

            process.exit();        
        }            
    } else {
        if(started == false)
            return;
        
        if(line != '') 
            output.push(line);
    }
})

// KernelTest test output parsing function
var kernelSuite = writer.addTestsuite('KernelTest');

var parse = function(exp, str) {
    var delimiter = /\[=*\]/;
    var testCases = str.split(delimiter);

    for(i = 0; i < testCases.length; i++) {
        var nameRegExp = new RegExp(exp['name'], ['g']);
        var passRegExp = new RegExp(exp['pass'], ['g']);
        var failRegExp = new RegExp(exp['fail'], ['g']);

        var name = nameRegExp.exec(testCases[i]);
        if(name == null)
            continue;

        // Index 1 of exec function is first captured item 
        name = name[1];
        var testCase = kernelSuite.addTestcase(name, 'KernelTest'); 

        var pass = testCases[i].match(passRegExp);
        if(pass == null) {
            var fail = failRegExp.exec(testCases[i])[1];
            testCase.addFailure(fail, 'KernelTest'); 
        } 
    }

    var casesRegExp = new RegExp(exp['cases'], ['g']);
    var successRegExp = new RegExp(exp['success'], ['g']);
    var errorRegExp = new RegExp(exp['errors'], ['g']);

    var cases = casesRegExp.exec(str)[1];
    var success = successRegExp.exec(str)[1];
    var error = errorRegExp.exec(str)[1];

    var result;
    if (cases == success) {
        result = 'PASS';
    } else {
        result = 'FAIL';
    }

    return result;
}

var verify = function(exp, cmd, str) {
    var result;
    if(typeof(exp) != 'string') {
        // Parse inner command recursively
        if(cmd == null)
            return 'ERROR';

        var nextCommand = cmd.next();
        if(nextCommand == undefined)
            return 'ERROR';

        return verify(exp[nextCommand], cmd, str);
    } else {
        var regExp = new RegExp(exp, ['gi']);
        // Expect console output
        console.log('[ EXPECTS ] ', regExp);
        var check = regExp.test(str);
        if(check) {
            result = 'PASS';
        } else {
            result = 'FAIL';
        }
    }

    return result;
}

// Test case count
var count = 0;
var shellSuite = writer.addTestsuite('ShellTest');

var test = function(cmd, str) {
    count++;
    console.log('-------------------- TEST CASE ' + count + ' --------------------'); 

    // Input command
    console.log('[ COMMAND ] ', cmd.list.join(' '));
    // Console output
    console.log('[ OUTPUTS ] ');
    console.log(str);

    var testCase = shellSuite.addTestcase(cmd.list.join(' '), 'ShellTest');
    var result;
    // KernelTest runtime test
    if(cmd.current() == 'test') {
        result = parse(cases.kernel[cmd.current()], str);
    } else {
    // Shell command test
        var exp = cases.shell[cmd.current()];
        if(exp == undefined) {
            result = 'ERROR'; 
        } else {
            result = verify(exp, cmd, str);
        }
    }

    console.log('[ RESULTS ] ', result);

    if(result == 'FAIL') {
        testCase.addFailure('Mismatch to expected output', 'Default type'); 
    } else if(result == 'ERROR') {
        testCase.addError('Failed to parse command', 'Default type'); 
    }

    console.log('\n');
}

