-- Base schema

CREATE TABLE Tables
(
  [Table] INTEGER NOT NULL,
  [Call1] CHAR(8) NOT NULL,
  [Call2] CHAR(8) NOT NULL,
  [Date] DATETIME NOT NULL,
  PRIMARY KEY([Table], [Call1])
);

CREATE TABLE Users
(
  [Nick] VARCHAR(16) PRIMARY KEY,
  [Name] VARCHAR(16),
  [Address] VARCHAR(64),
  [Enabled] INTEGER DEFAULT 1,
  [Server] VARCHAR(16) DEFAULT '',
  [Date] DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Extensions for high performance

CREATE TABLE Table0
(
  [Call1] CHAR(8) NOT NULL,
  [Call2] CHAR(8) NOT NULL,
  PRIMARY KEY([Call1])
);

CREATE TABLE Table1
(
  [Call1] CHAR(8) NOT NULL,
  [Call2] CHAR(8) NOT NULL,
  PRIMARY KEY([Call1])
);

CREATE TABLE Gateways
(
  [Call] CHAR(8) NOT NULL,
  [Address] VARCHAR(64) NOT NULL,
  PRIMARY KEY([Call])
);

CREATE INDEX GatewayAddress ON Gateways(Address);

CREATE TRIGGER RegisterStation AFTER INSERT ON Tables 
  WHEN (new.[Table] = 0)
BEGIN
  INSERT OR REPLACE
    INTO Table0 (Call1, Call2)
    VALUES (new.Call1, new.Call2);
END;

CREATE TRIGGER RegisterRepeater AFTER INSERT ON Tables 
  WHEN (new.[Table] = 1)
BEGIN
  INSERT OR REPLACE
    INTO Table1 (Call1, Call2)
    VALUES (new.Call1, new.Call2);
END;

CREATE TRIGGER RegisterGateway AFTER INSERT ON Users 
  WHEN (new.Enabled = 1) AND (new.Nick LIKE '%-_')
BEGIN
  INSERT OR REPLACE
    INTO Gateways (Call, Address)
    VALUES (substr(upper(new.Name) || '        ', 1, 8), new.Address);
END;

DROP VIEW Routes;

CREATE VIEW Routes AS
  SELECT 
      Table1.Call1 AS Call,
      'CQCQCQ  ' AS Station,
      Table1.Call1 AS Repeater,
      Table1.Call2 AS Gateway,
      Address
    FROM Table1
    JOIN Gateways ON Table1.Call2 = Gateways.Call
  UNION
  SELECT 
      Table0.Call1 AS Call,
      Table0.Call1 AS Station,
      Table0.Call2 AS Repeater,
      Table1.Call2 AS Gateway,
      Address
    FROM Table0
    JOIN Table1 ON Table0.Call2 = Table1.Call1
    JOIN Gateways ON Table1.Call2 = Gateways.Call;

-- Maintance

INSERT OR REPLACE INTO Table0 (Call1, Call2) SELECT Call1, Call2 FROM Tables WHERE [Table] = 0;
INSERT OR REPLACE INTO Table1 (Call1, Call2) SELECT Call1, Call2 FROM Tables WHERE [Table] = 1;

DELETE FROM Table0 WHERE Call1 IN (SELECT Call1 FROM Tables WHERE ([Table] = 0) AND ([Date] < '2012-08-01 00:00:00'));
