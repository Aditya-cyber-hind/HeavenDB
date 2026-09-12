-- ============================================
-- HeavenDB COMPLETE STRESS TEST
-- Every feature + Date/Time + JSON functions
-- Uses DROP TABLE IF EXISTS for clean first-run
-- ============================================

-- ============================================
-- SETUP
-- ============================================
DROP TABLE IF EXISTS events
DROP TABLE IF EXISTS transactions
DROP TABLE IF EXISTS products
DROP TABLE IF EXISTS categories
DROP TABLE IF EXISTS customers
DROP TABLE IF EXISTS orders
DROP TABLE IF EXISTS employees
DROP TABLE IF EXISTS departments
DROP TABLE IF EXISTS audit_log
DROP TABLE IF EXISTS users

CREATE TABLE users (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT NOT NULL, email TEXT, age INTEGER, salary FLOAT, is_active BOOLEAN, metadata JSON)
CREATE TABLE employees (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT NOT NULL, department_id INTEGER, salary FLOAT, hired_date DATE, active BOOLEAN)
CREATE TABLE departments (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT, budget FLOAT, location TEXT)
CREATE TABLE orders (id INTEGER PRIMARY KEY AUTO_INCREMENT, user_id INTEGER REFERENCES users(id) ON DELETE CASCADE, total FLOAT, status TEXT)
CREATE TABLE customers (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT, city TEXT, tier TEXT, lifetime_value FLOAT)
CREATE TABLE categories (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT, parent_id INTEGER)
CREATE TABLE products (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT, category_id INTEGER, price FLOAT, stock INTEGER, active BOOLEAN)
CREATE TABLE transactions (id INTEGER PRIMARY KEY AUTO_INCREMENT, product_id INTEGER, customer_id INTEGER, quantity INTEGER, total FLOAT, tx_date DATE)
CREATE TABLE audit_log (id INTEGER PRIMARY KEY AUTO_INCREMENT, action TEXT, details JSON, ts TIMESTAMP)
CREATE TABLE events (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT, event_date DATE, created_at TIMESTAMP)

-- ============================================
-- INSERT USERS
-- ============================================
INSERT INTO users VALUES (NULL, 'Aditya', 'aditya@heaven.db', 25, 85000.50, TRUE, '{"role":"engineer","level":3,"team":"backend"}')
INSERT INTO users VALUES (NULL, 'Rahul', 'rahul@heaven.db', 19, 65000.75, TRUE, '{"role":"designer","level":2,"team":"design"}')
INSERT INTO users VALUES (NULL, 'Priya', 'priya@heaven.db', 30, 95000.00, TRUE, '{"role":"manager","level":4,"team":"backend"}')
INSERT INTO users VALUES (NULL, 'Amit', 'amit@heaven.db', 45, 45000.25, FALSE, '{"role":"intern","level":1,"team":"backend"}')
INSERT INTO users VALUES (NULL, 'Sneha', 'sneha@heaven.db', 28, 75000.00, TRUE, '{"role":"engineer","level":3,"team":"frontend"}')
INSERT INTO users VALUES (NULL, 'Vikram', 'vikram@heaven.db', 35, 55000.50, TRUE, '{"role":"analyst","level":2,"team":"data"}')
INSERT INTO users VALUES (NULL, 'Neha', 'neha@heaven.db', 22, 48000.00, TRUE, '{"role":"engineer","level":2,"team":"frontend"}')
INSERT INTO users VALUES (NULL, 'Karan', 'karan@heaven.db', 40, 120000.00, TRUE, '{"role":"director","level":5,"team":"management"}')
INSERT INTO users VALUES (NULL, 'Ananya', 'ananya@heaven.db', 26, 72000.00, TRUE, '{"role":"engineer","level":3,"team":"backend"}')
INSERT INTO users VALUES (NULL, 'Rohan', 'rohan@heaven.db', 32, 68000.00, TRUE, '{"role":"designer","level":3,"team":"design"}')
INSERT INTO users VALUES (NULL, 'Divya', 'divya@heaven.db', 29, 81000.00, TRUE, '{"role":"engineer","level":3,"team":"backend"}')
INSERT INTO users VALUES (NULL, 'Arjun', 'arjun@heaven.db', 24, 58000.00, FALSE, '{"role":"analyst","level":2,"team":"data"}')
INSERT INTO users VALUES (NULL, 'Meera', 'meera@heaven.db', 31, 89000.00, TRUE, '{"role":"manager","level":4,"team":"frontend"}')
INSERT INTO users VALUES (NULL, 'Rohit', 'rohit@heaven.db', 27, 63000.00, TRUE, '{"role":"engineer","level":2,"team":"backend"}')
INSERT INTO users VALUES (NULL, 'Pooja', 'pooja@heaven.db', 33, 77000.00, TRUE, '{"role":"designer","level":3,"team":"design"}')

-- ============================================
-- INSERT DEPARTMENTS
-- ============================================
INSERT INTO departments VALUES (NULL, 'Engineering', 500000.00, 'Bangalore')
INSERT INTO departments VALUES (NULL, 'Design', 250000.00, 'Mumbai')
INSERT INTO departments VALUES (NULL, 'Sales', 300000.00, 'Delhi')
INSERT INTO departments VALUES (NULL, 'Marketing', 200000.00, 'Mumbai')
INSERT INTO departments VALUES (NULL, 'HR', 150000.00, 'Bangalore')
INSERT INTO departments VALUES (NULL, 'Finance', 180000.00, 'Delhi')
INSERT INTO departments VALUES (NULL, 'Operations', 220000.00, 'Pune')
INSERT INTO departments VALUES (NULL, 'Research', 350000.00, 'Bangalore')
INSERT INTO departments VALUES (NULL, 'Support', 160000.00, 'Chennai')
INSERT INTO departments VALUES (NULL, 'Legal', 140000.00, 'Delhi')

-- ============================================
-- INSERT EMPLOYEES
-- ============================================
INSERT INTO employees VALUES (NULL, 'Raj', 1, 90000.00, '2020-01-15', TRUE)
INSERT INTO employees VALUES (NULL, 'Simran', 1, 85000.00, '2021-03-20', TRUE)
INSERT INTO employees VALUES (NULL, 'Akash', 1, 95000.00, '2019-07-10', TRUE)
INSERT INTO employees VALUES (NULL, 'Riya', 2, 70000.00, '2022-05-12', TRUE)
INSERT INTO employees VALUES (NULL, 'Nikhil', 2, 75000.00, '2021-08-25', TRUE)
INSERT INTO employees VALUES (NULL, 'Tanvi', 3, 65000.00, '2023-01-05', TRUE)
INSERT INTO employees VALUES (NULL, 'Manav', 3, 68000.00, '2022-11-18', TRUE)
INSERT INTO employees VALUES (NULL, 'Isha', 4, 60000.00, '2023-02-28', FALSE)
INSERT INTO employees VALUES (NULL, 'Dev', 4, 62000.00, '2022-09-15', TRUE)
INSERT INTO employees VALUES (NULL, 'Sara', 5, 58000.00, '2023-04-10', TRUE)
INSERT INTO employees VALUES (NULL, 'Yash', 5, 55000.00, '2023-06-22', TRUE)
INSERT INTO employees VALUES (NULL, 'Diya', 6, 72000.00, '2021-12-01', TRUE)
INSERT INTO employees VALUES (NULL, 'Kabir', 7, 80000.00, '2020-10-30', TRUE)
INSERT INTO employees VALUES (NULL, 'Naina', 8, 105000.00, '2018-05-15', TRUE)
INSERT INTO employees VALUES (NULL, 'Aryan', 8, 98000.00, '2019-11-20', TRUE)
INSERT INTO employees VALUES (NULL, 'Zara', 9, 45000.00, '2023-07-01', TRUE)
INSERT INTO employees VALUES (NULL, 'Ishaan', 9, 48000.00, '2023-08-15', TRUE)
INSERT INTO employees VALUES (NULL, 'Kavya', 10, 65000.00, '2022-03-10', TRUE)
INSERT INTO employees VALUES (NULL, 'Veer', 10, 68000.00, '2021-06-20', TRUE)
INSERT INTO employees VALUES (NULL, 'Aisha', 1, 110000.00, '2017-09-05', TRUE)

-- ============================================
-- INSERT CATEGORIES
-- ============================================
INSERT INTO categories VALUES (NULL, 'Electronics', 0)
INSERT INTO categories VALUES (NULL, 'Computers', 1)
INSERT INTO categories VALUES (NULL, 'Phones', 1)
INSERT INTO categories VALUES (NULL, 'Clothing', 0)
INSERT INTO categories VALUES (NULL, 'Books', 0)

-- ============================================
-- INSERT PRODUCTS
-- ============================================
INSERT INTO products VALUES (NULL, 'Laptop Pro', 2, 85000.00, 50, TRUE)
INSERT INTO products VALUES (NULL, 'Laptop Air', 2, 65000.00, 75, TRUE)
INSERT INTO products VALUES (NULL, 'Desktop Gaming', 2, 120000.00, 20, TRUE)
INSERT INTO products VALUES (NULL, 'iPhone 15', 3, 80000.00, 100, TRUE)
INSERT INTO products VALUES (NULL, 'Samsung S24', 3, 70000.00, 150, TRUE)
INSERT INTO products VALUES (NULL, 'Pixel 8', 3, 60000.00, 80, TRUE)
INSERT INTO products VALUES (NULL, 'T-Shirt Premium', 4, 1500.00, 500, TRUE)
INSERT INTO products VALUES (NULL, 'Jeans Classic', 4, 2500.00, 300, TRUE)
INSERT INTO products VALUES (NULL, 'Jacket Winter', 4, 5000.00, 100, FALSE)
INSERT INTO products VALUES (NULL, 'Python Book', 5, 800.00, 200, TRUE)
INSERT INTO products VALUES (NULL, 'C Programming', 5, 1200.00, 150, TRUE)
INSERT INTO products VALUES (NULL, 'System Design', 5, 1500.00, 100, TRUE)
INSERT INTO products VALUES (NULL, 'Headphones Pro', 1, 25000.00, 60, TRUE)
INSERT INTO products VALUES (NULL, 'Smart Watch', 1, 35000.00, 40, TRUE)
INSERT INTO products VALUES (NULL, 'Camera DSLR', 1, 95000.00, 15, TRUE)

-- ============================================
-- INSERT CUSTOMERS
-- ============================================
INSERT INTO customers VALUES (NULL, 'Ramesh', 'Mumbai', 'Gold', 250000.00)
INSERT INTO customers VALUES (NULL, 'Suresh', 'Delhi', 'Silver', 120000.00)
INSERT INTO customers VALUES (NULL, 'Mahesh', 'Bangalore', 'Platinum', 500000.00)
INSERT INTO customers VALUES (NULL, 'Dinesh', 'Pune', 'Gold', 180000.00)
INSERT INTO customers VALUES (NULL, 'Naresh', 'Chennai', 'Bronze', 45000.00)
INSERT INTO customers VALUES (NULL, 'Krish', 'Mumbai', 'Platinum', 750000.00)
INSERT INTO customers VALUES (NULL, 'Arun', 'Delhi', 'Gold', 220000.00)
INSERT INTO customers VALUES (NULL, 'Varun', 'Bangalore', 'Silver', 95000.00)
INSERT INTO customers VALUES (NULL, 'Tarun', 'Pune', 'Bronze', 35000.00)
INSERT INTO customers VALUES (NULL, 'Karun', 'Chennai', 'Gold', 310000.00)

-- ============================================
-- INSERT ORDERS
-- ============================================
INSERT INTO orders VALUES (NULL, 1, 5000.00, 'completed')
INSERT INTO orders VALUES (NULL, 1, 7500.00, 'completed')
INSERT INTO orders VALUES (NULL, 2, 3000.00, 'pending')
INSERT INTO orders VALUES (NULL, 3, 12000.00, 'completed')
INSERT INTO orders VALUES (NULL, 4, 4500.00, 'cancelled')
INSERT INTO orders VALUES (NULL, 5, 8900.00, 'completed')
INSERT INTO orders VALUES (NULL, 6, 2300.00, 'pending')
INSERT INTO orders VALUES (NULL, 7, 6700.00, 'completed')
INSERT INTO orders VALUES (NULL, 8, 15000.00, 'completed')
INSERT INTO orders VALUES (NULL, 9, 3200.00, 'pending')
INSERT INTO orders VALUES (NULL, 10, 5500.00, 'completed')
INSERT INTO orders VALUES (NULL, 11, 9100.00, 'cancelled')
INSERT INTO orders VALUES (NULL, 12, 4300.00, 'completed')
INSERT INTO orders VALUES (NULL, 13, 7800.00, 'pending')
INSERT INTO orders VALUES (NULL, 14, 2100.00, 'completed')
INSERT INTO orders VALUES (NULL, 15, 6400.00, 'completed')

-- ============================================
-- INSERT TRANSACTIONS
-- ============================================
INSERT INTO transactions VALUES (NULL, 1, 1, 2, 170000.00, '2026-01-15')
INSERT INTO transactions VALUES (NULL, 4, 2, 1, 80000.00, '2026-01-16')
INSERT INTO transactions VALUES (NULL, 7, 3, 5, 7500.00, '2026-01-17')
INSERT INTO transactions VALUES (NULL, 10, 4, 3, 2400.00, '2026-01-18')
INSERT INTO transactions VALUES (NULL, 13, 5, 2, 50000.00, '2026-01-19')
INSERT INTO transactions VALUES (NULL, 2, 6, 1, 65000.00, '2026-02-01')
INSERT INTO transactions VALUES (NULL, 5, 7, 2, 140000.00, '2026-02-02')
INSERT INTO transactions VALUES (NULL, 8, 8, 4, 10000.00, '2026-02-03')
INSERT INTO transactions VALUES (NULL, 11, 9, 5, 7500.00, '2026-02-04')
INSERT INTO transactions VALUES (NULL, 14, 10, 1, 35000.00, '2026-02-05')
INSERT INTO transactions VALUES (NULL, 3, 1, 1, 120000.00, '2026-02-10')
INSERT INTO transactions VALUES (NULL, 6, 2, 3, 180000.00, '2026-02-11')
INSERT INTO transactions VALUES (NULL, 9, 3, 2, 5000.00, '2026-02-12')
INSERT INTO transactions VALUES (NULL, 12, 4, 1, 1500.00, '2026-02-13')
INSERT INTO transactions VALUES (NULL, 15, 5, 1, 95000.00, '2026-02-14')
INSERT INTO transactions VALUES (NULL, 1, 6, 2, 170000.00, '2026-03-01')
INSERT INTO transactions VALUES (NULL, 4, 7, 3, 240000.00, '2026-03-02')
INSERT INTO transactions VALUES (NULL, 7, 8, 4, 6000.00, '2026-03-03')
INSERT INTO transactions VALUES (NULL, 10, 9, 5, 4000.00, '2026-03-04')
INSERT INTO transactions VALUES (NULL, 13, 10, 2, 50000.00, '2026-03-05')

-- ============================================
-- INSERT AUDIT LOG
-- ============================================
INSERT INTO audit_log VALUES (NULL, 'CREATE_USER', '{"user":"admin","ip":"127.0.0.1"}', '2026-09-12 10:30:00')
INSERT INTO audit_log VALUES (NULL, 'LOGIN', '{"user":"admin","success":true}', '2026-09-12 10:31:15')
INSERT INTO audit_log VALUES (NULL, 'BACKUP', '{"file":"stress_backup.hdb","size":"1MB"}', '2026-09-12 10:45:00')

-- ============================================
-- INSERT EVENTS
-- ============================================
INSERT INTO events VALUES (NULL, 'Team Meeting', '2026-09-15', '2026-09-12 14:30:00')
INSERT INTO events VALUES (NULL, 'Product Launch', '2026-10-20', '2026-09-10 09:00:00')
INSERT INTO events VALUES (NULL, 'Birthday Party', '2026-12-25', NOW())
INSERT INTO events VALUES (NULL, 'Conference', '2026-11-05', '2026-09-12 08:00:00')
INSERT INTO events VALUES (NULL, 'Vacation', '2026-12-31', NOW())

-- ============================================
-- STRESS 1: SELECT * on all tables
-- ============================================
SELECT * FROM users
SELECT * FROM departments
SELECT * FROM employees
SELECT * FROM categories
SELECT * FROM products
SELECT * FROM customers
SELECT * FROM orders
SELECT * FROM transactions
SELECT * FROM audit_log
SELECT * FROM events

-- ============================================
-- STRESS 2: WHERE operators
-- ============================================
SELECT * FROM users WHERE age > 25
SELECT * FROM users WHERE age < 25
SELECT * FROM users WHERE salary > 80000
SELECT * FROM users WHERE is_active = TRUE
SELECT * FROM users WHERE is_active = FALSE

-- ============================================
-- STRESS 3: AND / OR
-- ============================================
SELECT * FROM users WHERE age > 20 AND salary > 70000
SELECT * FROM users WHERE age < 25 OR salary > 100000

-- ============================================
-- STRESS 4: ORDER BY
-- ============================================
SELECT name, salary FROM users ORDER BY salary ASC
SELECT name, salary FROM users ORDER BY salary DESC
SELECT name, age FROM users ORDER BY age DESC

-- ============================================
-- STRESS 5: LIMIT / OFFSET
-- ============================================
SELECT * FROM users LIMIT 5
SELECT * FROM users LIMIT 5 OFFSET 5

-- ============================================
-- STRESS 6: DISTINCT
-- ============================================
SELECT DISTINCT age FROM users
SELECT DISTINCT active FROM employees

-- ============================================
-- STRESS 7: Aggregates
-- ============================================
SELECT COUNT(*) FROM users
SELECT COUNT(*) FROM employees
SELECT COUNT(*) FROM products
SELECT COUNT(*) FROM orders
SELECT COUNT(*) FROM transactions
SELECT COUNT(*) FROM events
SELECT SUM(salary) FROM users
SELECT AVG(salary) FROM users
SELECT MIN(salary) FROM users
SELECT MAX(salary) FROM users
SELECT GROUP_CONCAT(name) FROM users

-- ============================================
-- STRESS 8: GROUP BY / HAVING
-- ============================================
SELECT is_active, COUNT(*) FROM users GROUP BY is_active
SELECT status, COUNT(*) FROM orders GROUP BY status
SELECT tier, COUNT(*) FROM customers GROUP BY tier
SELECT age, COUNT(*) FROM users GROUP BY age HAVING COUNT(*) > 1
SELECT is_active, GROUP_CONCAT(name) FROM users GROUP BY is_active

-- ============================================
-- STRESS 9: LIKE / BETWEEN / IN
-- ============================================
SELECT * FROM users WHERE name LIKE 'A%'
SELECT * FROM users WHERE age BETWEEN 25 AND 35
SELECT * FROM users WHERE id IN (1, 3, 5, 7, 9)
SELECT * FROM orders WHERE status IN ('completed', 'pending')

-- ============================================
-- STRESS 10: JOINS
-- ============================================
SELECT * FROM users INNER JOIN orders ON users.id = orders.user_id
SELECT * FROM users LEFT JOIN orders ON users.id = orders.user_id
SELECT * FROM users RIGHT JOIN orders ON users.id = orders.user_id
SELECT * FROM users FULL JOIN orders ON users.id = orders.user_id
SELECT * FROM employees INNER JOIN departments ON employees.department_id = departments.id

-- ============================================
-- STRESS 11: CROSS JOIN + UNION
-- ============================================
SELECT * FROM users CROSS JOIN orders
SELECT name FROM users UNION SELECT name FROM employees
SELECT name FROM users UNION ALL SELECT name FROM customers

-- ============================================
-- STRESS 12: String functions
-- ============================================
SELECT UPPER(name) FROM users
SELECT LOWER(name) FROM users
SELECT LENGTH(name) FROM users
SELECT TRIM('   hello   ')
SELECT SUBSTR('HeavenDB is awesome', 1, 8)
SELECT CONCAT('User: ', name, ' - Salary: ', salary) FROM users
SELECT UPPER(CONCAT(name, ' from ', email)) FROM users

-- ============================================
-- STRESS 13: Math functions
-- ============================================
SELECT ABS(-12345)
SELECT ROUND(3.14159, 2)
SELECT FLOOR(99.999)
SELECT CEIL(0.001)
SELECT MOD(1000, 13)

-- ============================================
-- STRESS 14: CASE WHEN
-- ============================================
SELECT name, CASE WHEN salary > 80000 THEN 'high' ELSE 'normal' END FROM users
SELECT name, CASE WHEN age < 25 THEN 'young' ELSE 'adult' END FROM users

-- ============================================
-- STRESS 15: EXISTS / NOT EXISTS
-- ============================================
SELECT * FROM users WHERE EXISTS (SELECT * FROM orders WHERE orders.user_id = users.id)
SELECT * FROM users WHERE NOT EXISTS (SELECT * FROM orders WHERE orders.user_id = users.id)

-- ============================================
-- STRESS 16: Scalar subqueries
-- ============================================
SELECT * FROM users WHERE age > (SELECT AVG(age) FROM users)
SELECT * FROM users WHERE age = (SELECT MAX(age) FROM users)
SELECT * FROM employees WHERE salary > (SELECT AVG(salary) FROM employees)
SELECT * FROM products WHERE price > (SELECT AVG(price) FROM products)

-- ============================================
-- STRESS 17: ANY / ALL
-- ============================================
SELECT * FROM users WHERE salary > ANY (SELECT salary FROM employees)
SELECT * FROM users WHERE salary > ALL (SELECT salary FROM employees)

-- ============================================
-- STRESS 18: JSON functions
-- ============================================
SELECT json_extract(metadata, 'role') FROM users
SELECT json_extract(metadata, 'level') FROM users
SELECT json_extract(metadata, 'team') FROM users
SELECT json_set(metadata, 'role', 'manager') FROM users
SELECT json_set(metadata, 'level', '9') FROM users

-- ============================================
-- STRESS 19: Date/Time functions
-- ============================================
SELECT NOW()
SELECT CURRENT_DATE()
SELECT * FROM events
SELECT * FROM events WHERE event_date > '2026-10-01'
SELECT * FROM events WHERE event_date BETWEEN '2026-09-01' AND '2026-11-30'
SELECT YEAR(event_date) FROM events
SELECT MONTH(event_date) FROM events
SELECT DAY(event_date) FROM events
SELECT YEAR(hired_date) FROM employees
SELECT MONTH(hired_date) FROM employees

-- ============================================
-- STRESS 20: Transactions
-- ============================================
BEGIN
INSERT INTO users VALUES (NULL, 'StressUser1', 'stress1@test.com', 99, 1000.00, TRUE, '{}')
COMMIT

BEGIN
INSERT INTO users VALUES (NULL, 'RollbackUser', 'rollback@test.com', 97, 3000.00, TRUE, '{}')
ROLLBACK

SELECT * FROM users WHERE name LIKE 'Stress%'
SELECT * FROM users WHERE name LIKE 'Rollback%'

-- ============================================
-- STRESS 21: SAVEPOINT
-- ============================================
BEGIN
INSERT INTO users VALUES (NULL, 'SP_User1', 'sp1@test.com', 100, 100.00, TRUE, '{}')
SAVEPOINT checkpoint
INSERT INTO users VALUES (NULL, 'SP_User2', 'sp2@test.com', 101, 200.00, TRUE, '{}')
ROLLBACK TO SAVEPOINT checkpoint
COMMIT

SELECT * FROM users WHERE name LIKE 'SP_%'

-- ============================================
-- STRESS 22: Views
-- ============================================
CREATE VIEW high_earners AS SELECT name, salary FROM users WHERE salary > 70000
SELECT * FROM high_earners
CREATE VIEW upcoming_events AS SELECT name, event_date FROM events WHERE event_date > '2026-09-15'
SELECT * FROM upcoming_events
DROP VIEW upcoming_events
DROP VIEW high_earners

-- ============================================
-- STRESS 23: ALTER TABLE
-- ============================================
ALTER TABLE users ADD COLUMN phone TEXT
SELECT * FROM users WHERE id = 1
ALTER TABLE users DROP COLUMN phone
SELECT * FROM users WHERE id = 1

-- ============================================
-- STRESS 24: UPDATE
-- ============================================
UPDATE users SET salary = 90000.00 WHERE id = 1
UPDATE users SET is_active = FALSE WHERE id = 4
UPDATE employees SET active = FALSE WHERE department_id = 5

-- ============================================
-- STRESS 25: FK CASCADE
-- ============================================
SELECT * FROM orders WHERE user_id = 2
DELETE FROM users WHERE id = 2
SELECT * FROM orders WHERE user_id = 2

-- ============================================
-- STRESS 26: DELETE
-- ============================================
DELETE FROM transactions WHERE id > 15
SELECT COUNT(*) FROM transactions
DELETE FROM orders WHERE status = 'cancelled'
SELECT * FROM orders

-- ============================================
-- STRESS 27: Admin commands
-- ============================================
SHOW TABLES
DESCRIBE users
DESCRIBE events
DESCRIBE transactions
EXPLAIN SELECT * FROM users WHERE salary > 70000
EXPLAIN SELECT * FROM events WHERE event_date > '2026-09-15'

-- ============================================
-- STRESS 28: Indexes
-- ============================================
CREATE INDEX idx_user_salary ON users(salary)
CREATE INDEX idx_user_age ON users(age)
CREATE INDEX idx_event_date ON events(event_date)

-- ============================================
-- STRESS 29: BACKUP
-- ============================================
BACKUP TO 'stress_backup.hdb'

-- ============================================
-- FINAL TALLY
-- ============================================
SELECT COUNT(*) FROM users
SELECT COUNT(*) FROM employees
SELECT COUNT(*) FROM departments
SELECT COUNT(*) FROM products
SELECT COUNT(*) FROM customers
SELECT COUNT(*) FROM orders
SELECT COUNT(*) FROM transactions
SELECT COUNT(*) FROM events
SELECT COUNT(*) FROM audit_log