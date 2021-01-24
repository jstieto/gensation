var http = require('http'); // 1 - Import Node.js core module

var server = http.createServer(function (req, res) {   // 2 - creating server
    if (req.url == '/') { //check the URL of the current request
        res.writeHead(200, { 'Content-Type': 'text/html' }); 
        res.write('<html><body><p>You did not specify a path-parameter to echo back.</p></body></html>');
        res.end();    
    }
    else {
		var len = req.url.length
		var rurl = req.url.substring(1, len);
		res.writeHead(200, { 'Content-Type': 'text/html' }); 
        res.write(rurl);
        res.end();
	}
});

server.listen(5000); //3 - listen for any incoming requests

console.log('Node.js web server at port 5000 is running..')