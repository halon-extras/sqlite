# SQLite client plugin

This plugin is a wrapper around [sqlite3](https://sqlite.org/cintro.html).

## Installation

Follow the [instructions](https://docs.halon.io/manual/comp_install.html#installation) in our manual to add our package repository and then run the below command.

### Ubuntu

```
apt-get install halon-extras-sqlite
```

### RHEL

```
yum install halon-extras-sqlite
```

## Configuration

For the configuration schema, see [sqlite-app.schema.json](sqlite-app.schema.json).

### smtpd-app.yaml

```
plugins:
  - id: sqlite
    config:
      default_profile: sqlite
      profiles:
        - id: sqlite
          filename: /tmp/database.db
          pool_size: 32
          foreign_keys: true
          readonly: false
```

## Exported classes

These classes needs to be [imported](https://docs.halon.io/hsl/structures.html#import) from the `extras://sqlite` module path.

### SQLite([string $profile])

The SQLite class is a [sqlite3](https://sqlite.org/cintro.html) wrapper class. On error an exception is thrown.

#### query(string $query [, array $params]): array

Pass a "prepared" statement to SQLite. On error an exception is thrown. See the table below for the data types mapping.

| SQLite           | HSL      |
|------------------|----------|
| `SQLITE_TEXT`    | `string` |
| `SQLITE_INTEGER` | `string` |
| `SQLITE_FLOAT`   | `number` |
| `SQLITE_BLOB`    | `string` |
| `SQLITE_NULL`    | `none`   |

## Examples

```
import { SQLite } from "extras://sqlite";

// Create "SQLite" instance with an (optional) config profile
$sqlite = SQLite("sqlite");
$sqlite->query("CREATE TABLE IF NOT EXISTS employee (name VARCHAR(255), salary INT, working BOOLEAN, age SMALLINT)");
$sqlite->query("INSERT INTO employee (name, salary, working, age) VALUES (?, ?, ?, ?)", ["John Doe", 54321, true, 36]);
$sqlite->query("INSERT INTO employee (name, salary, working, age) VALUES (?, ?, ?, ?)", ["Jane Doe", 54321, false, 36]);
echo $sqlite->query("SELECT * FROM employee");
// [0=>["name"=>"John Doe","salary"=>"54321","working"=>"1","age"=>"36"],1=>["name"=>"Jane Doe","salary"=>"54321","working"=>"0","age"=>"36"]]
```
