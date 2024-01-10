CREATE TABLE users (
    login VARCHAR(256) PRIMARY KEY,
    password VARCHAR(256),
    salt VARCHAR(256),
    first_name VARCHAR(256),
    last_name VARCHAR(256),
    email VARCHAR(256),
    permission VARCHAR(32),
    change_pwd_uuid INT,
    change_pwd_date DATETIME
)
