/* 
	 Oracle script for generating the tables used by the unit tests
   Ignore the script errors at the beginning, if the tables don't
   exist in your schema
*/

-- Drop exisiting tables
ALTER TABLE LISTMEMBER DROP PRIMARY KEY CASCADE;
DROP TABLE LISTMEMBER CASCADE CONSTRAINTS;

ALTER TABLE LIST DROP PRIMARY KEY CASCADE;
DROP TABLE LIST CASCADE CONSTRAINTS;

ALTER TABLE MEMBERPICTURE DROP PRIMARY KEY CASCADE;
DROP TABLE MEMBERPICTURE CASCADE CONSTRAINTS;

ALTER TABLE GUIDHOLDER DROP PRIMARY KEY CASCADE;
DROP TABLE GUIDHOLDER CASCADE CONSTRAINTS;

ALTER TABLE PROPERTYHOLDER DROP PRIMARY KEY CASCADE;
DROP TABLE PROPERTYHOLDER CASCADE CONSTRAINTS;

ALTER TABLE USERROLES DROP PRIMARY KEY CASCADE;
DROP TABLE USERROLES CASCADE CONSTRAINTS;

ALTER TABLE ROLES DROP PRIMARY KEY CASCADE;
DROP TABLE ROLES CASCADE CONSTRAINTS;

ALTER TABLE USERS DROP PRIMARY KEY CASCADE;
DROP TABLE USERS CASCADE CONSTRAINTS;

ALTER TABLE MULTITYPE DROP PRIMARY KEY CASCADE;
DROP TABLE MULTITYPE CASCADE CONSTRAINTS;

-- Drop exisiting sequences
DROP SEQUENCE LIST_SEQ;
DROP SEQUENCE LISTMEMBER_SEQ;
DROP SEQUENCE MEMBERPICTURE_SEQ;
DROP SEQUENCE PROPERTYHOLDER_SEQ;
DROP SEQUENCE USERS_SEQ;
DROP SEQUENCE ROLES_SEQ;
DROP SEQUENCE MULTITYPE_SEQ;


-- Create the tables
CREATE TABLE LIST
(
  LISTID         NUMBER(10) NOT NULL,
  LISTNAME       VARCHAR2(4000) NOT NULL,
  SENDERADDRESS  VARCHAR2(255) NOT NULL,
  CONSTRAINT PK_LIST PRIMARY KEY (LISTID)
);

CREATE TABLE LISTMEMBER
(
  MEMBERID       NUMBER(10) NOT NULL,
  LISTID         NUMBER(10) NOT NULL,
  MEMBERNAME     VARCHAR2(4000),
  MEMBERADDRESS  VARCHAR2(255) NOT NULL,
  DATABASEVERSION NUMBER(10) DEFAULT 1 NOT NULL,
  CONSTRAINT PK_LISTMEMBER PRIMARY KEY (MEMBERID),
  CONSTRAINT FK_LISTMEMBER_LIST FOREIGN KEY (LISTID) 
    REFERENCES LIST (LISTID)
);

CREATE TABLE MEMBERPICTURE
(
  PICTUREID    NUMBER(10),
  MEMBERID     NUMBER(10),
  PICTUREDATA  BLOB,
  CONSTRAINT PK_MEMBERPICTURE PRIMARY KEY (PICTUREID),
  CONSTRAINT MEMBERPICTURE_LISTMEMBER FOREIGN KEY (MEMBERID) 
    REFERENCES LISTMEMBER (MEMBERID)
);

CREATE TABLE GUIDHOLDER
(
  GUID       VARCHAR2(16),
  SOMEVALUE  NUMBER(10),
  CONSTRAINT PK_GUIDHOLDER PRIMARY KEY (GUID)
);


CREATE TABLE PROPERTYHOLDER
(
  PH_ID      NUMBER(10) NOT NULL,
  PH_NAME    VARCHAR2(4000) NOT NULL,
  TINT       NUMBER(5),
  TLONG		 NUMBER(20),
  TDECIMAL   NUMBER(38,3),
  TDOUBLE    FLOAT(126),
  TBOOL      NUMBER(1),
  TDATETIME  DATE,
  TDATETIMENN  DATE NOT NULL,
  TCHAR      CHAR(8),
  TNCHAR     NCHAR(8),
  TVARCHAR   VARCHAR2(8),
  TNVARCHAR  NVARCHAR2(8),
  TTEXT      VARCHAR2(4000),
  TNTEXT     NVARCHAR2(2000),
  CONSTRAINT PK_PROPERTYHOLDER PRIMARY KEY (PH_ID)
);

CREATE TABLE ROLES
(
  ROLEID     NUMBER(10) NOT NULL,
  ROLENAME   VARCHAR2(255) NOT NULL,
  CONSTRAINT PK_ROLES PRIMARY KEY (ROLEID)
);

CREATE TABLE USERS
(
  USERID     NUMBER(10) NOT NULL,
  FIRSTNAME  VARCHAR2(255) NOT NULL,
  LASTNAME   VARCHAR2(255) NOT NULL,
  PRIMARYROLE NUMBER(10) NOT NULL,
  CONSTRAINT PK_USERS PRIMARY KEY (USERID)
);

CREATE TABLE USERROLES
(
  USERID    NUMBER(10) NOT NULL,
  ROLEID    NUMBER(10) NOT NULL,
  MEMBERID  NUMBER(10),
  CONSTRAINT PK_USERROLES PRIMARY KEY (USERID, ROLEID),
  CONSTRAINT FK_USERROLES_ROLES FOREIGN KEY (ROLEID) 
    REFERENCES ROLES (ROLEID),
  CONSTRAINT FK_USERROLES_USERS FOREIGN KEY (USERID) 
    REFERENCES USERS (USERID),
  CONSTRAINT FK_USERROLES_LISTMEMBER FOREIGN KEY (MEMBERID) 
    REFERENCES LISTMEMBER (MEMBERID)
);


CREATE TABLE MULTITYPE
(
  ID NUMBER(10) NOT NULL,
  TYPE VARCHAR2(250) NOT NULL,
  FIELD1 NUMBER(10),
  FIELD2 NUMBER(38,2),
  FIELD3 FLOAT,
  FIELD4 VARCHAR2(4000),
  CONSTRAINT PK_MULTITYPE PRIMARY KEY (ID )
);


-- Add test data
INSERT INTO LIST ( LISTID, LISTNAME, SENDERADDRESS ) VALUES ( 1, 'Announce', 'ann-sender@foobar.com' ); 
INSERT INTO LIST ( LISTID, LISTNAME, SENDERADDRESS ) VALUES ( 2, 'Discussion', 'first.foobar.com' ); 
INSERT INTO LIST ( LISTID, LISTNAME, SENDERADDRESS ) VALUES ( 3, 'Messages', 'info-sender@foobar.org' ); 
commit;
 
INSERT INTO LISTMEMBER ( MEMBERID, LISTID, MEMBERNAME, MEMBERADDRESS, DATABASEVERSION ) VALUES ( 1, 1, 'First User', 'first@foobar.com', 1 ); 
INSERT INTO LISTMEMBER ( MEMBERID, LISTID, MEMBERNAME, MEMBERADDRESS, DATABASEVERSION ) VALUES ( 2, 2, 'First User', 'first@foobar.com', 1 ); 
INSERT INTO LISTMEMBER ( MEMBERID, LISTID, MEMBERNAME, MEMBERADDRESS, DATABASEVERSION ) VALUES ( 3, 1, 'Second User', 'second@bar.com', 1 ); 
INSERT INTO LISTMEMBER ( MEMBERID, LISTID, MEMBERNAME, MEMBERADDRESS, DATABASEVERSION ) VALUES ( 4, 3, 'Third User', 'third@foo.org', 1 ); 
commit;
 
 
-- Create sequences for identity columns
CREATE SEQUENCE LISTMEMBER_SEQ INCREMENT BY 1 START WITH 5 MAXVALUE 2147483647 NOCYCLE NOCACHE NOORDER;
CREATE SEQUENCE LIST_SEQ INCREMENT BY 1 START WITH 4 MAXVALUE 2147483647 NOCYCLE NOCACHE NOORDER;
CREATE SEQUENCE MEMBERPICTURE_SEQ INCREMENT BY 1 START WITH 1 MINVALUE 1 MAXVALUE 2147483647 NOCYCLE NOCACHE NOORDER;
CREATE SEQUENCE PROPERTYHOLDER_SEQ INCREMENT BY 1 START WITH 1 MAXVALUE 2147483647 NOCYCLE NOCACHE NOORDER;
CREATE SEQUENCE USERS_SEQ INCREMENT BY 1 START WITH 1 MAXVALUE 2147483647 NOCYCLE NOCACHE NOORDER;
CREATE SEQUENCE ROLES_SEQ INCREMENT BY 1 START WITH 1 MAXVALUE 2147483647 NOCYCLE NOCACHE NOORDER;
CREATE SEQUENCE MULTITYPE_SEQ INCREMENT BY 1 START WITH 1 MAXVALUE 2147483647 NOCYCLE NOCACHE NOORDER;


-- Create triggers for automated identity increment
CREATE OR REPLACE TRIGGER TRG_LIST_BI
BEFORE INSERT
ON LIST 
REFERENCING NEW AS NEW OLD AS OLD
FOR EACH ROW
BEGIN
   SELECT LIST_SEQ.NEXTVAL INTO :NEW.LISTID FROM dual;
END ;
/
SHOW ERRORS;

CREATE OR REPLACE TRIGGER TRG_LISTMEMBER_BI
BEFORE INSERT
ON LISTMEMBER 
REFERENCING NEW AS NEW OLD AS OLD
FOR EACH ROW
BEGIN
   SELECT LISTMEMBER_SEQ.NEXTVAL INTO :NEW.MEMBERID FROM dual;
END ;
/
SHOW ERRORS;

CREATE OR REPLACE TRIGGER TRG_MEMBERPICTURE_BI
BEFORE INSERT
ON MEMBERPICTURE 
REFERENCING NEW AS NEW OLD AS OLD
FOR EACH ROW
BEGIN
   SELECT MEMBERPICTURE_SEQ.NEXTVAL INTO :NEW.PICTUREID FROM dual;
END ;
/
SHOW ERRORS;

CREATE OR REPLACE TRIGGER TRG_PROPERTYHOLDER_BI
BEFORE INSERT
ON PROPERTYHOLDER 
REFERENCING NEW AS NEW OLD AS OLD
FOR EACH ROW
BEGIN
   SELECT PROPERTYHOLDER_SEQ.NEXTVAL INTO :NEW.PH_ID FROM dual;
END ;
/
SHOW ERRORS;

CREATE OR REPLACE TRIGGER TR_USERS_BI
BEFORE INSERT
ON USERS 
REFERENCING NEW AS NEW OLD AS OLD
FOR EACH ROW
BEGIN
   SELECT USERS_SEQ.NEXTVAL INTO :NEW.USERID FROM dual;
END ;
/
SHOW ERRORS;

CREATE OR REPLACE TRIGGER TR_ROLES_BI
BEFORE INSERT
ON ROLES 
REFERENCING NEW AS NEW OLD AS OLD
FOR EACH ROW
BEGIN
SELECT ROLES_SEQ.NEXTVAL INTO :NEW.ROLEID FROM dual;
END ;
/
SHOW ERRORS;

CREATE OR REPLACE TRIGGER TR_MULTITYPE_BI
BEFORE INSERT
ON MULTITYPE 
REFERENCING NEW AS NEW OLD AS OLD
FOR EACH ROW
BEGIN
SELECT MULTITYPE_SEQ.NEXTVAL INTO :NEW.ID FROM dual;
END ;
/
SHOW ERRORS;
