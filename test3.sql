-- ============================================
-- HeavenDB STRESS TEST
-- Every feature + heavy data
-- ============================================

-- ============================================
-- SETUP: Drop everything
-- ============================================
DROP TABLE transactions
DROP TABLE products
DROP TABLE categories
DROP TABLE customers
DROP TABLE orders
DROP TABLE employees
DROP TABLE departments
DROP TABLE audit_log
DROP TABLE users

-- ============================================
-- CREATE TABLES (all types + constraints)
-- ============================================
CREATE TABLE users (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT NOT NULL, email TEXT, age INTEGER, salary FLOAT, is_active BOOLEAN, metadata JSON)
CREATE TABLE employees (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT NOT NULL, department_id INTEGER, salary FLOAT, hired_date TEXT, active BOOLEAN)
CREATE TABLE departments (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT, budget FLOAT, location TEXT)
CREATE TABLE orders (id INTEGER PRIMARY KEY AUTO_INCREMENT, user_id INTEGER REFERENCES users(id) ON DELETE CASCADE, total FLOAT, status TEXT)
CREATE TABLE customers (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT, city TEXT, tier TEXT, lifetime_value FLOAT)
CREATE TABLE categories (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT, parent_id INTEGER)
CREATE TABLE products (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT, category_id INTEGER, price FLOAT, stock INTEGER, active BOOLEAN)
CREATE TABLE transactions (id INTEGER PRIMARY KEY AUTO_INCREMENT, product_id INTEGER, customer_id INTEGER, quantity INTEGER, total FLOAT, tx_date TEXT)
CREATE TABLE audit_log (id INTEGER PRIMARY KEY AUTO_INCREMENT, action TEXT, details JSON, ts TEXT)

-- ============================================
-- INSERT: 15 users
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
-- INSERT: 10 departments
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
-- INSERT: 20 employees
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
-- INSERT: 5 categories
-- ============================================
INSERT INTO categories VALUES (NULL, 'Electronics', 0)
INSERT INTO categories VALUES (NULL, 'Computers', 1)
INSERT INTO categories VALUES (NULL, 'Phones', 1)
INSERT INTO categories VALUES (NULL, 'Clothing', 0)
INSERT INTO categories VALUES (NULL, 'Books', 0)

-- ============================================
-- INSERT: 15 products
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
-- INSERT: 10 customers
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
-- INSERT: 16 orders
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
-- INSERT: 20 transactions
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
-- INSERT: audit log
-- ============================================
INSERT INTO audit_log VALUES (NULL, 'CREATE_USER', '{"user":"admin","ip":"127.0.0.1"}', '2026-09-12')
INSERT INTO audit_log VALUES (NULL, 'LOGIN', '{"user":"admin","success":true}', '2026-09-12')
INSERT INTO audit_log VALUES (NULL, 'BACKUP', '{"file":"stress_backup.hdb","size":"1MB"}', '2026-09-12')

-- ============================================
-- STRESS 1: Simple SELECT on all tables
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

-- ============================================
-- STRESS 2: WHERE with all operators
-- ============================================
SELECT * FROM users WHERE age > 25
SELECT * FROM users WHERE age < 25
SELECT * FROM users WHERE age >= 30
SELECT * FROM users WHERE age <= 30
SELECT * FROM users WHERE age = 25
SELECT * FROM users WHERE age != 25
SELECT * FROM users WHERE salary > 80000
SELECT * FROM users WHERE is_active = TRUE
SELECT * FROM users WHERE is_active = FALSE

-- ============================================
-- STRESS 3: AND / OR combos
-- ============================================
SELECT * FROM users WHERE age > 20 AND salary > 70000
SELECT * FROM users WHERE age < 25 OR salary > 100000
SELECT * FROM users WHERE is_active = TRUE AND age > 25 AND salary > 70000

-- ============================================
-- STRESS 4: ORDER BY
-- ============================================
SELECT name, salary FROM users ORDER BY salary ASC
SELECT name, salary FROM users ORDER BY salary DESC
SELECT name, age FROM users ORDER BY age ASC
SELECT name, age FROM users ORDER BY age DESC

-- ============================================
-- STRESS 5: LIMIT / OFFSET
-- ============================================
SELECT * FROM users LIMIT 5
SELECT * FROM users LIMIT 5 OFFSET 5
SELECT * FROM users LIMIT 5 OFFSET 10

-- ============================================
-- STRESS 6: DISTINCT
-- ============================================
SELECT DISTINCT age FROM users
SELECT DISTINCT active FROM employees

-- ============================================
-- STRESS 7: All aggregates
-- ============================================
SELECT COUNT(*) FROM users
SELECT COUNT(*) FROM employees
SELECT COUNT(*) FROM products
SELECT COUNT(*) FROM customers
SELECT COUNT(*) FROM orders
SELECT COUNT(*) FROM transactions
SELECT SUM(salary) FROM users
SELECT AVG(salary) FROM users
SELECT MIN(salary) FROM users
SELECT MAX(salary) FROM users
SELECT SUM(total) FROM orders
SELECT AVG(total) FROM orders
SELECT SUM(budget) FROM departments
SELECT AVG(budget) FROM departments
SELECT GROUP_CONCAT(name) FROM users
SELECT GROUP_CONCAT(name) FROM departments

-- ============================================
-- STRESS 8: GROUP BY
-- ============================================
SELECT is_active, COUNT(*) FROM users GROUP BY is_active
SELECT age, COUNT(*) FROM users GROUP BY age
SELECT status, COUNT(*) FROM orders GROUP BY status
SELECT tier, COUNT(*) FROM customers GROUP BY tier
SELECT city, COUNT(*) FROM customers GROUP BY city
SELECT category_id, COUNT(*) FROM products GROUP BY category_id

-- ============================================
-- STRESS 9: GROUP_CONCAT with GROUP BY
-- ============================================
SELECT is_active, GROUP_CONCAT(name) FROM users GROUP BY is_active
SELECT status, GROUP_CONCAT(total) FROM orders GROUP BY status

-- ============================================
-- STRESS 10: HAVING
-- ============================================
SELECT age, COUNT(*) FROM users GROUP BY age HAVING COUNT(*) > 1
SELECT status, COUNT(*) FROM orders GROUP BY status HAVING COUNT(*) > 5

-- ============================================
-- STRESS 11: LIKE
-- ============================================
SELECT * FROM users WHERE name LIKE 'A%'
SELECT * FROM users WHERE name LIKE '%a'
SELECT * FROM users WHERE email LIKE '%heaven.db'
SELECT * FROM products WHERE name LIKE '%Pro%'

-- ============================================
-- STRESS 12: BETWEEN
-- ============================================
SELECT name, age FROM users WHERE age BETWEEN 25 AND 35
SELECT name, salary FROM users WHERE salary BETWEEN 50000 AND 80000
SELECT name, price FROM products WHERE price BETWEEN 10000 AND 80000

-- ============================================
-- STRESS 13: IN
-- ============================================
SELECT * FROM users WHERE id IN (1, 3, 5, 7, 9)
SELECT * FROM products WHERE category_id IN (1, 2, 3)
SELECT * FROM orders WHERE status IN ('completed', 'pending')

-- ============================================
-- STRESS 14: JOINS
-- ============================================
SELECT * FROM users INNER JOIN orders ON users.id = orders.user_id
SELECT * FROM users LEFT JOIN orders ON users.id = orders.user_id
SELECT * FROM users RIGHT JOIN orders ON users.id = orders.user_id
SELECT * FROM users FULL JOIN orders ON users.id = orders.user_id
SELECT * FROM employees INNER JOIN departments ON employees.department_id = departments.id
SELECT * FROM employees LEFT JOIN departments ON employees.department_id = departments.id
SELECT * FROM products INNER JOIN categories ON products.category_id = categories.id
SELECT * FROM transactions INNER JOIN products ON transactions.product_id = products.id
SELECT * FROM transactions INNER JOIN customers ON transactions.customer_id = customers.id

-- ============================================
-- STRESS 15: CROSS JOIN
-- ============================================
SELECT * FROM users CROSS JOIN orders

-- ============================================
-- STRESS 16: UNION
-- ============================================
SELECT name FROM users UNION SELECT name FROM employees
SELECT name FROM users UNION ALL SELECT name FROM customers
SELECT name FROM departments UNION SELECT name FROM categories

-- ============================================
-- STRESS 17: String functions
-- ============================================
SELECT UPPER(name) FROM users
SELECT LOWER(name) FROM users
SELECT LENGTH(name) FROM users
SELECT TRIM('   hello   ')
SELECT SUBSTR('HeavenDB is awesome', 1, 8)
SELECT CONCAT('User: ', name, ' - Salary: ', salary) FROM users
SELECT UPPER(CONCAT(name, ' from ', email)) FROM users

-- ============================================
-- STRESS 18: Math functions
-- ============================================
SELECT ABS(-12345)
SELECT ABS(-0.5)
SELECT ROUND(3.14159)
SELECT ROUND(3.14159, 2)
SELECT ROUND(3.14159, 4)
SELECT FLOOR(99.999)
SELECT CEIL(0.001)
SELECT MOD(100, 7)
SELECT MOD(1000, 13)

-- ============================================
-- STRESS 19: CASE WHEN
-- ============================================
SELECT name, CASE WHEN salary > 80000 THEN 'high' ELSE 'normal' END FROM users
SELECT name, CASE WHEN age < 25 THEN 'young' ELSE 'adult' END FROM users

-- ============================================
-- STRESS 20: EXISTS
-- ============================================
SELECT * FROM users WHERE EXISTS (SELECT * FROM orders WHERE orders.user_id = users.id)
SELECT * FROM users WHERE NOT EXISTS (SELECT * FROM orders WHERE orders.user_id = users.id)
SELECT * FROM customers WHERE EXISTS (SELECT * FROM transactions WHERE transactions.customer_id = customers.id)

-- ============================================
-- STRESS 21: Scalar subqueries
-- ============================================
SELECT * FROM users WHERE age > (SELECT AVG(age) FROM users)
SELECT * FROM users WHERE age = (SELECT MAX(age) FROM users)
SELECT * FROM employees WHERE salary > (SELECT AVG(salary) FROM employees)
SELECT * FROM products WHERE price > (SELECT AVG(price) FROM products)
SELECT * FROM orders WHERE total > (SELECT AVG(total) FROM orders)

-- ============================================
-- STRESS 22: ANY / ALL (using salary - employees has salary)
-- ============================================
SELECT * FROM users WHERE salary > ANY (SELECT salary FROM employees)
SELECT * FROM users WHERE salary > ALL (SELECT salary FROM employees)
SELECT * FROM products WHERE price > ANY (SELECT price FROM products)
SELECT * FROM products WHERE price > ALL (SELECT price FROM products)

-- ============================================
-- STRESS 23: Transactions
-- ============================================
BEGIN
INSERT INTO users VALUES (NULL, 'StressUser1', 'stress1@test.com', 99, 1000.00, TRUE, '{}')
INSERT INTO users VALUES (NULL, 'StressUser2', 'stress2@test.com', 98, 2000.00, TRUE, '{}')
COMMIT

BEGIN
INSERT INTO users VALUES (NULL, 'RollbackUser', 'rollback@test.com', 97, 3000.00, TRUE, '{}')
ROLLBACK

SELECT * FROM users WHERE name LIKE 'Stress%'
SELECT * FROM users WHERE name LIKE 'Rollback%'

-- ============================================
-- STRESS 24: SAVEPOINT
-- ============================================
BEGIN
INSERT INTO users VALUES (NULL, 'SP_User1', 'sp1@test.com', 100, 100.00, TRUE, '{}')
SAVEPOINT checkpoint
INSERT INTO users VALUES (NULL, 'SP_User2', 'sp2@test.com', 101, 200.00, TRUE, '{}')
ROLLBACK TO SAVEPOINT checkpoint
COMMIT

SELECT * FROM users WHERE name LIKE 'SP_%'

-- ============================================
-- STRESS 25: Views
-- ============================================
CREATE VIEW high_earners AS SELECT name, salary FROM users WHERE salary > 70000
SELECT * FROM high_earners
CREATE VIEW active_users AS SELECT name, age, is_active FROM users WHERE is_active = TRUE
SELECT * FROM active_users
CREATE VIEW expensive_products AS SELECT name, price FROM products WHERE price > 50000
SELECT * FROM expensive_products
DROP VIEW expensive_products
DROP VIEW active_users
DROP VIEW high_earners

-- ============================================
-- STRESS 26: ALTER TABLE
-- ============================================
ALTER TABLE users ADD COLUMN phone TEXT
SELECT * FROM users WHERE id = 1
ALTER TABLE users DROP COLUMN phone
SELECT * FROM users WHERE id = 1

-- ============================================
-- STRESS 27: UPDATE
-- ============================================
UPDATE users SET salary = 90000.00 WHERE id = 1
UPDATE users SET is_active = FALSE WHERE id = 4
UPDATE employees SET active = FALSE WHERE department_id = 5
SELECT * FROM users WHERE id = 1
SELECT * FROM employees WHERE department_id = 5

-- ============================================
-- STRESS 28: FK CASCADE
-- ============================================
SELECT * FROM orders WHERE user_id = 2
DELETE FROM users WHERE id = 2
SELECT * FROM orders WHERE user_id = 2

-- ============================================
-- STRESS 29: DELETE
-- ============================================
DELETE FROM transactions WHERE id > 15
SELECT COUNT(*) FROM transactions
DELETE FROM orders WHERE status = 'cancelled'
SELECT * FROM orders

-- ============================================
-- STRESS 30: Admin commands
-- ============================================
SHOW TABLES
DESCRIBE users
DESCRIBE employees
DESCRIBE products
DESCRIBE transactions
EXPLAIN SELECT * FROM users WHERE salary > 70000
EXPLAIN SELECT * FROM products WHERE price > 50000

-- ============================================
-- STRESS 31: Indexes
-- ============================================
CREATE INDEX idx_user_salary ON users(salary)
CREATE INDEX idx_user_age ON users(age)
CREATE INDEX idx_product_price ON products(price)
CREATE INDEX idx_product_name ON products(name)

-- ============================================
-- STRESS 32: BACKUP
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