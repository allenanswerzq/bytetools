const app = require('express')();
const bodyParser = require('body-parser');

const port = 3333;

app.use(bodyParser.json());

app.post('/', (req, res) => {
  const data = req.body;

  console.log(`Problem name: ${data.name}`);
  console.log(`Problem group: ${data.group}`);
  console.log('Full body:');
  console.log(JSON.stringify(data, null, 4));
  res.sendStatus(200);
  var i, final = "";
  for (i = 0; i < data.tests.length; i++) {
    var inp = data.tests[i].input.trim().split("\n");
    // console.log(inp);
    for (j = 0; j < inp.length; j++) {
      if (inp[j].trim() == "") {
      // Handle the case where the input has a blank line.
      // Intended add `//n` to represent a blank line.
        inp[j] = "//n";
      }
    }
    var input = "";
    for (j = 0; j < inp.length; j++) {
      input += inp[j];
      input += "\n";
    }
    console.log(inp, input);
    final += "input\n";
    final += input;
    var out = data.tests[i].output.trim().split("\n");
    // console.log(out);
    for (j = 0; j < out.length; j++) {
      if (out[j].trim() == "") {
        out[j] = "//n";
      }
    }
    var output = "";
    for (j = 0; j < out.length; j++) {
      output += out[j];
      output += "\n";
    }
    console.log(out, output);
    final += "output\n";
    final += output;
  }

  write_name = '/tmp/algo-samples';
  if (data.group == 'Codeforces - submission error') {
    write_name = '/tmp/algo-wrong'
  }
  const fs = require('fs');
  fs.writeFile(write_name, final, function(err) {
    if (err) {
      return console.log(err);
    }
    console.log(`The Problem {{${data.name}}} is saved.\n`);
  });

});

app.listen(port, err => {
  if (err) {
    console.error(err);
    process.exit(1);
  }

  console.log(`Listening on port ${port}`);
});
