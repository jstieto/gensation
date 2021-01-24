/**
 *  This script runs a web server that allows:
 *    * file upload using a form with password
 *    * file download of previously uploaded files with password
 */

var http = require('http');
var formidable = require('formidable');
var fs = require('fs');
var url = require('url');

// Handling forms with file-upload - using "formidable": https://www.w3schools.com/nodejs/nodejs_uploadfiles.asp
// Run a node.js server forever - using "forever": https://github.com/foreverjs/forever

var uploadFoldername = '/uploads';
var uploadPath = __dirname + uploadFoldername;

var expectedPassword = "checkremu"

var port = 8080;
//port = process.env.PORT;
console.log("Starting server on port " + port);

http.createServer(function (req, res) {
  if (req.url == '/fileupload') {
    console.log("a file is being uploaded!");
    res.writeHead(200, {'Content-Type': 'text/html'});
    res.write('<html><body><head><meta charset="UTF-8"></head>');

    var form = new formidable.IncomingForm();
    form.parse(req, function (err, fields, files) {
      if(!err){
        var enteredPassword = fields.password;              // TODO: re-enable password-check
        if (enteredPassword == expectedPassword) {
          console.log("eingegebenes pwd = " + enteredPassword);
          var oldpath = files.filetoupload.path;
          var newpath = uploadPath + "/" + files.filetoupload.name;
          fs.rename(oldpath, newpath, function (err) {
            if (err) throw err;
            res.write('File uploaded and moved to ' + newpath + '!<br>');
            res.write('</body></html>');
            res.end();
          });
        }
        else {
          res.write("Falsches Passwort angegeben. File-Upload nicht möglich!");
          res.write('</body></html>');
          res.end();
        }
      }
      else {
                                console.dir(err);
      }
    });
  }
  else if (req.url.match('\/filedownload\?(.*)')) {                             // example: .../filedownload?filename=uni_linz_ggg.png&password=xYz
    console.log("a file is being requested to download!");
    var q = url.parse(req.url, true).query;
    var enteredPassword = q.password;
    var requestedFilename = uploadPath + "/" + q.filename;

    if (enteredPassword == expectedPassword) {
      console.log("correct pwd");
      if (fs.existsSync(requestedFilename)) {
        console.log("serving file " + requestedFilename);
        fs.readFile(requestedFilename,function(err,contents){
                                if(!err){
                                        //if there was no error
                                        //send the contents with the default 200/ok header
                                        res.writeHead(200,{
                                                "Content-type" : "image/png",
                                                "Content-Length" : contents.length
                                        });
                                        res.end(contents);
                                } else {
                                        console.dir(err);
                                };
                        });
      }
      else {
        console.log("Die Datei " + requestedFilename + " existiert nicht!");
        res.writeHead(200, {'Content-Type': 'text/plain' });
        res.write(" ohoh ");
        res.end('404 Angeforderte Datei nicht gefunden');
      }
    }
    else {
      res.writeHead(200, {'Content-Type': 'text/plain' });
      res.write(" ohoh ");
      res.end('404 - falsches Passwort ');
    }
  }
  else {
    console.log("general script start!");
    res.writeHead(200, {'Content-Type': 'text/html'});
    res.write('<html><body><head><meta charset="UTF-8"></head>');
    res.write('Wähle deine hochzuladende Datei und gib dein Passwort ein:<br>');
    res.write('  <form action="fileupload" method="post" enctype="multipart/form-data">');
    res.write('    <input type="file" name="filetoupload"><br>');
    res.write('    <input type="password" name="password" value=""><br>');
    res.write('    <input type="submit">');
    res.write('  </form>');
    res.write('</body></html>');
    return res.end();
  }
}).listen(port);

