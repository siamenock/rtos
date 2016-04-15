// Test cases set
var cases = require('./case.js');
// Readline interface
var readline = require('readline');
var read = readline.createInterface({
      input: process.stdin,
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

        var index = line.search(/[A-Za-z]/);
        if(index > 0) 
            command = new Command(line.substring(index).split(" "));
        
    } else {
        if(started == false)
            return;
        
        if(line != '') 
            output.push(line);
    }
})

var regEx = function(exp, cmd, str) {
    var result;
    if(typeof(exp) != 'string') {
        // Parse inner command recursively
        var nextCommand = cmd.next();
        if(nextCommand == undefined)
            return 'ERROR : Command parsing error. Regular expression should be String formatted';

        return regEx(exp[nextCommand], cmd, str);
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
var test = function(cmd, str) {
    count++;
    console.log('-------------------- TEST CASE ' + count + ' --------------------'); 

    // Input command
    console.log('[ COMMAND ] ', cmd.current());
    // Console output
    console.log('[ OUTPUTS ] ');
    console.log(str);

    var exp = cases[cmd.current()];
    var result;
    if(exp == undefined) 
        result = 'ERROR : Command "' + cmd.current() + '" not found';
    else 
        result = regEx(exp, cmd, str);

    // Test result
    console.log('[ RESULTS ] ', result);
    console.log('\n');
}
