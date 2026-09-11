-- ============================================
-- HeavenDB Complete Feature Test
-- ============================================

-- ============================================
-- 1. AUTHENTICATION
-- ============================================
LOGIN admin WITH PASSWORD 'admin123'
CREATE USER testuser WITH PASSWORD 'Test1234'
LOGOUT

-- ============================================
-- 2. TABLE CREATION
-- ============================================
DROP TABLE users
DROP TABLE orders
DROP TABLE admins

CREATE TABLE users (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT NOT NULL, age INTEGER, active BOOLEAN, data JSON)
CREATE TABLE orders (id INTEGER PRIMARY KEY AUTO_INCREMENT, user_id INTEGER REFERENCES users(id), total INTEGER)
CREATE TABLE admins (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT, level INTEGER)

-- ============================================
-- 3. INSERT
-- ============================================
INSERT INTO users VALUES (NULL, 'Aditya', 25, TRUE, '{"city":"Mumbai"}')
INSERT INTO users VALUES (NULL, 'Rahul', 19, FALSE, '{"city":"Delhi"}')
INSERT INTO users VALUES (NULL, 'Priya', 30, TRUE, '{"city":"Bangalore"}')
INSERT INTO users VALUES (NULL, 'Test', 25, TRUE, '{"city":"Pune"}')
INSERT INTO users VALUES (NULL, 'Test2', 30, FALSE, '{"city":"Chennai"}')
INSERT INTO orders VALUES (NULL, 1, 500)
INSERT INTO orders VALUES (NULL, 2, 750)
INSERT INTO orders VALUES (NULL, 99, 999)
INSERT INTO admins VALUES (NULL, 'SuperAdmin', 1)

-- ============================================
-- 4. SELECT
-- ============================================
SELECT * FROM users
SELECT name, age FROM users
SELECT name, age, active FROM users

-- ============================================
-- 5. WHERE OPERATORS
-- ============================================
SELECT * FROM users WHERE age = 25
SELECT * FROM users WHERE age != 25
SELECT * FROM users WHERE age <> 25
SELECT * FROM users WHERE age > 20
SELECT * FROM users WHERE age < 30
SELECT * FROM users WHERE age >= 25
SELECT * FROM users WHERE age <= 25
SELECT * FROM users WHERE name = 'Aditya'
SELECT * FROM users WHERE active = TRUE
SELECT * FROM users WHERE active = FALSE

-- ============================================
-- 6. AND / OR
-- ============================================
SELECT * FROM users WHERE age > 20 AND age < 30
SELECT * FROM users WHERE name = 'Aditya' OR name = 'Priya'
SELECT * FROM users WHERE age > 20 AND active = TRUE

-- ============================================
-- 7. ORDER BY
-- ============================================
SELECT * FROM users ORDER BY age ASC
SELECT * FROM users ORDER BY age DESC

-- ============================================
-- 8. LIMIT / OFFSET
-- ============================================
SELECT * FROM users LIMIT 2
SELECT * FROM users LIMIT 2 OFFSET 2

-- ============================================
-- 9. DISTINCT
-- ============================================
SELECT DISTINCT age FROM users

-- ============================================
-- 10. AGGREGATES
-- ============================================
SELECT COUNT(*) FROM users
SELECT SUM(age) FROM users
SELECT AVG(age) FROM users
SELECT MIN(age) FROM users
SELECT MAX(age) FROM users
SELECT GROUP_CONCAT(name) FROM users

-- ============================================
-- 11. GROUP BY / HAVING
-- ============================================
SELECT age, COUNT(*) FROM users GROUP BY age
SELECT age, GROUP_CONCAT(name) FROM users GROUP BY age
SELECT age, COUNT(*) FROM users GROUP BY age HAVING COUNT(*) > 1

-- ============================================
-- 12. LIKE / BETWEEN / IN / IS NULL
-- ============================================
SELECT * FROM users WHERE name LIKE 'Adi%'
SELECT * FROM users WHERE name LIKE 'Test%'
SELECT * FROM users WHERE age BETWEEN 20 AND 30
SELECT * FROM users WHERE age IN (25, 30)

-- ============================================
-- 13. JOINS
-- ============================================
SELECT * FROM users INNER JOIN orders ON users.id = orders.user_id
SELECT * FROM users LEFT JOIN orders ON users.id = orders.user_id
SELECT * FROM users RIGHT JOIN orders ON users.id = orders.user_id
SELECT * FROM users FULL JOIN orders ON users.id = orders.user_id
SELECT * FROM users CROSS JOIN orders

-- ============================================
-- 14. UNION
-- ============================================
SELECT name FROM users UNION SELECT name FROM admins
SELECT name FROM users UNION ALL SELECT name FROM admins

-- ============================================
-- 15. STRING FUNCTIONS
-- ============================================
SELECT UPPER('hello')
SELECT LOWER('HELLO')
SELECT LENGTH('hello')
SELECT TRIM('  spaces  ')
SELECT SUBSTR('Hello World', 1, 5)
SELECT CONCAT('Hello', ' ', 'World')
SELECT UPPER(name) FROM users
SELECT LOWER(name) FROM users
SELECT LENGTH(name) FROM users
SELECT CONCAT('Mr. ', name, ' - Age: ', age) FROM users

-- ============================================
-- 16. MATH FUNCTIONS
-- ============================================
SELECT ABS(-5)
SELECT ROUND(3.14159)
SELECT ROUND(3.14159, 2)
SELECT FLOOR(3.9)
SELECT CEIL(3.1)
SELECT MOD(10, 3)

-- ============================================
-- 17. UPDATE
-- ============================================
UPDATE users SET age = 26 WHERE id = 1
UPDATE users SET name = 'Aditya Kumar' WHERE id = 1
SELECT * FROM users WHERE id = 1
UPDATE users SET active = FALSE WHERE id = 2
SELECT * FROM users WHERE id = 2

-- ============================================
-- 18. DELETE
-- ============================================
DELETE FROM users WHERE id = 5
SELECT * FROM users
DELETE FROM admins WHERE id = 1
SELECT * FROM admins

-- ============================================
-- 19. TRANSACTIONS
-- ============================================
BEGIN
INSERT INTO users VALUES (NULL, 'TransactionTest', 99, TRUE, '{}')
COMMIT
SELECT * FROM users WHERE name = 'TransactionTest'

BEGIN
INSERT INTO users VALUES (NULL, 'RollbackTest', 88, TRUE, '{}')
ROLLBACK
SELECT * FROM users WHERE name = 'RollbackTest'

-- ============================================
-- 20. VIEWS (CREATE, SELECT, DROP)
-- ============================================
CREATE VIEW adult_users AS SELECT * FROM users WHERE age >= 18
SELECT * FROM adult_users
DROP VIEW adult_users
CREATE VIEW young_users AS SELECT * FROM users WHERE age < 18
SELECT * FROM young_users
DROP VIEW young_users

-- ============================================
-- 21. CREATE INDEX
-- ============================================
CREATE INDEX idx_age ON users(age)
CREATE INDEX idx_name ON users(name)

-- ============================================
-- 22. ADMIN COMMANDS
-- ============================================
SHOW TABLES
DESCRIBE users
EXPLAIN SELECT * FROM users WHERE age > 18

-- ============================================
-- 23. BACKUP
-- ============================================
BACKUP TO 'test_backup.hdb'

-- ============================================
-- 24. FINAL STATE
-- ============================================
SELECT * FROM users
SELECT COUNT(*) FROM users