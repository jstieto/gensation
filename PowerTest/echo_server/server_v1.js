const { Pool } = require('pg')
var http = require('http'); // 1 - Import Node.js core module
var url = require('url');


const pool = new Pool({
  user: 'pegelalarm',
  host: 'dev.pegelalarm.at',
  database: 'dev_pegelalarm',
  password: 'wePostgres11',
  port: 5432
})
pool.connect()
		

var server = http.createServer(function (req, res) {   // 2 - creating server
    if (req.url == '/') { //check the URL of the current request
        res.writeHead(200, { 'Content-Type': 'text/html' }); 
        res.write('<html><body><p>You did not specify a path-parameter to echo back.</p></body></html>');
        res.end();
    }
    else if (req.url.match('\/echo\?(.*)')) {                             // example: .../echo/...
		var len = req.url.length
		var rurl = req.url.substring(6, len);
		res.writeHead(200, { 'Content-Type': 'text/html' }); 
        res.write(rurl);
        res.end();
	}
    else if (req.url.match('\/save\?(.*)')) {                             // example: .../save?username=joe&counter=123
		var q = url.parse(req.url, true).query;		
		const text = 'INSERT INTO gensation.signals (username, counter) VALUES ($1, $2) RETURNING *'
		const values = [q.username, q.counter]
		pool.query(text, values, (err, resu) => {
		  console.log(err, resu)
		})
		res.writeHead(200, { 'Content-Type': 'text/html' }); 
        res.write("Saved values to DB!");
        res.end();
	}
    else if (req.url.match('\/getmax\?(.*)')) {                             // example: .../getmax?username=joe
		var q = url.parse(req.url, true).query;		
		//const select = "SELECT max(counter) as cnt FROM gensation.signals where username = '" + q.username + "'"
		const select = "SELECT counter, ts FROM gensation.signals where username = '" + q.username + "' AND counter = (select max(counter) from gensation.signals where username = '" + q.username + "')"
		result = ""
		pool.query(select, (err, resu) => {
		    if (err) {
				console.log(err.stack)
		    } else {
				res.writeHead(200, { 'Content-Type': 'text/html' }); 
				if (resu.rows.length == 1) {
					res.write("Max value of user " + q.username + " is <b>" + resu.rows[0].counter + "</b> and was sent at <b>" + resu.rows[0].ts + "</b>.");
				}
				else {
					res.write("No data saved for user " + q.username);
				}
				res.end();				
			}
		})
	}
    else {		
		res.writeHead(200, { 'Content-Type': 'text/html' }); 
        res.write('<html><body><p>Want an echo? Try .../echo/[your_value].</p><p>Want to save a value for a user/device? Try .../save?username=[username]&counter=[a number]</body></html>');
        res.end();
	}
});

server.listen(5000); //3 - listen for any incoming requests

console.log('Node.js web server at port 5000 is running..')