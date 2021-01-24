const { Pool, Client } = require('pg')
const pool = new Pool({
  user: 'pegelalarm',
  host: 'dev.pegelalarm.at',
  database: 'dev_pegelalarm',
  password: 'wePostgres11',
  port: 5432,
})
pool.query('SELECT NOW()', (err, res) => {
  console.log(err, res)
  pool.end()
})
const client = new Client({
  user: 'pegelalarm',
  host: 'dev.pegelalarm.at',
  database: 'dev_pegelalarm',
  password: 'wePostgres11',
  port: 5432
})
client.connect()
client.query('SELECT NOW()', (err, res) => {
  console.log(err, res)
  client.end()
})