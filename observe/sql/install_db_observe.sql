CREATE TABLE daemons (
    id INT NOT NULL AUTO_INCREMENT,
    host VARCHAR(256),
    port INT,
    daemon VARCHAR(256),
    date TIMESTAMP NOT NULL,
    PRIMARY KEY (id)
);

INSERT INTO daemons SET host='primula', port=8888, daemon='spectrographd';
INSERT INTO daemons SET host='primula', port=9999, daemon='telescoped';
INSERT INTO daemons SET host='primula', port=443, daemon='observed';
INSERT INTO daemons SET host='sulafat', port=8888, daemon='spectrographd';
INSERT INTO daemons SET host='sulafat', port=9999, daemon='telescoped';
INSERT INTO daemons SET host='primula', port=5000, daemon='exposed-frodo';
INSERT INTO daemons SET host='almisan', port=5001, daemon='exposed-sauron';
INSERT INTO daemons SET host='almisan', port=5002, daemon='exposed-gandalf';
