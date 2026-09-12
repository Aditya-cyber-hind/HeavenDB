-- ============================================
-- HeavenDB Test Suite #2 - Advanced Features
-- ============================================

-- ============================================
-- 1. AUTH - SETUP
-- ============================================
LOGIN admin WITH PASSWORD 'admin123'

-- ============================================
-- 2. CLEANUP
-- ============================================
DROP TABLE employees
DROP TABLE departments
DROP TABLE projects
DROP TABLE assignments
DROP TABLE audit_log

-- ============================================
-- 3. CREATE TABLES
-- ============================================
CREATE TABLE employees (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT NOT NULL, email TEXT, salary FLOAT, is_active BOOLEAN, metadata JSON)
CREATE TABLE departments (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT, budget FLOAT, location TEXT)
CREATE TABLE projects (id INTEGER PRIMARY KEY AUTO_INCREMENT, name TEXT, department_id INTEGER, status TEXT)
CREATE TABLE assignments (id INTEGER PRIMARY KEY AUTO_INCREMENT, employee_id INTEGER, project_id INTEGER, hours INTEGER)
CREATE TABLE audit_log (id INTEGER PRIMARY KEY AUTO_INCREMENT, action TEXT, details JSON)

-- ============================================
-- 4. INSERT EMPLOYEES
-- ============================================
INSERT INTO employees VALUES (NULL, 'Aditya', 'aditya@heaven.db', 85000.50, TRUE, '{"role":"engineer","level":3}')
INSERT INTO employees VALUES (NULL, 'Rahul', 'rahul@heaven.db', 65000.75, TRUE, '{"role":"designer","level":2}')
INSERT INTO employees VALUES (NULL, 'Priya', 'priya@heaven.db', 95000.00, TRUE, '{"role":"manager","level":4}')
INSERT INTO employees VALUES (NULL, 'Amit', 'amit@heaven.db', 45000.25, FALSE, '{"role":"intern","level":1}')
INSERT INTO employees VALUES (NULL, 'Sneha', 'sneha@heaven.db', 75000.00, TRUE, '{"role":"engineer","level":3}')
INSERT INTO employees VALUES (NULL, 'Vikram', 'vikram@heaven.db', 55000.50, TRUE, '{"role":"analyst","level":2}')

-- ============================================
-- 5. INSERT DEPARTMENTS
-- ============================================
INSERT INTO departments VALUES (NULL, 'Engineering', 500000.00, 'Bangalore')
INSERT INTO departments VALUES (NULL, 'Design', 250000.00, 'Mumbai')
INSERT INTO departments VALUES (NULL, 'Sales', 300000.00, 'Delhi')

-- ============================================
-- 6. INSERT PROJECTS
-- ============================================
INSERT INTO projects VALUES (NULL, 'HeavenDB', 1, 'active')
INSERT INTO projects VALUES (NULL, 'UI Redesign', 2, 'active')
INSERT INTO projects VALUES (NULL, 'Q4 Campaign', 3, 'planning')
INSERT INTO projects VALUES (NULL, 'Legacy Migration', 1, 'completed')

-- ============================================
-- 7. INSERT ASSIGNMENTS
-- ============================================
INSERT INTO assignments VALUES (NULL, 1, 1, 160)
INSERT INTO assignments VALUES (NULL, 2, 2, 120)
INSERT INTO assignments VALUES (NULL, 3, 1, 80)
INSERT INTO assignments VALUES (NULL, 5, 1, 200)
INSERT INTO assignments VALUES (NULL, 6, 3, 100)

-- ============================================
-- 8. INSERT AUDIT
-- ============================================
INSERT INTO audit_log VALUES (NULL, 'CREATE_USER', '{"user":"admin","timestamp":"2026-09-11"}')
INSERT INTO audit_log VALUES (NULL, 'LOGIN', '{"user":"admin","ip":"127.0.0.1"}')
INSERT INTO audit_log VALUES (NULL, 'BACKUP', '{"file":"test_backup.hdb"}')

-- ============================================
-- 9. BASIC SELECTS
-- ============================================
SELECT * FROM employees
SELECT name, email FROM employees
SELECT name, salary, is_active FROM employees
SELECT * FROM departments
SELECT * FROM projects
SELECT * FROM assignments
SELECT * FROM audit_log

-- ============================================
-- 10. FLOAT TESTS
-- ============================================
SELECT name, salary FROM employees WHERE salary > 70000
SELECT name, salary FROM employees WHERE salary < 60000
SELECT name, salary FROM employees WHERE salary >= 75000
SELECT name, salary FROM employees WHERE salary <= 55000

-- ============================================
-- 11. BOOLEAN TESTS
-- ============================================
SELECT * FROM employees WHERE is_active = TRUE
SELECT * FROM employees WHERE is_active = FALSE

-- ============================================
-- 12. TEXT TESTS
-- ============================================
SELECT * FROM employees WHERE name = 'Aditya'
SELECT * FROM employees WHERE email = 'rahul@heaven.db'

-- ============================================
-- 13. COMPLEX AND / OR
-- ============================================
SELECT * FROM employees WHERE salary > 60000 AND is_active = TRUE
SELECT * FROM employees WHERE name = 'Aditya' OR name = 'Priya'
SELECT * FROM employees WHERE is_active = TRUE AND salary > 70000

-- ============================================
-- 14. ORDER BY
-- ============================================
SELECT name, salary FROM employees ORDER BY salary ASC
SELECT name, salary FROM employees ORDER BY salary DESC

-- ============================================
-- 15. LIMIT / OFFSET
-- ============================================
SELECT * FROM employees LIMIT 3
SELECT * FROM employees LIMIT 3 OFFSET 2

-- ============================================
-- 16. AGGREGATES ON FLOAT
-- ============================================
SELECT COUNT(*) FROM employees
SELECT SUM(salary) FROM employees
SELECT AVG(salary) FROM employees
SELECT MIN(salary) FROM employees
SELECT MAX(salary) FROM employees

-- ============================================
-- 17. GROUP_CONCAT
-- ============================================
SELECT GROUP_CONCAT(name) FROM employees

-- ============================================
-- 18. STRING FUNCTIONS ON COLUMNS
-- ============================================
SELECT UPPER(name) FROM employees
SELECT LOWER(name) FROM employees
SELECT LENGTH(name) FROM employees
SELECT LENGTH(email) FROM employees
SELECT CONCAT(name, ' <', email, '>') FROM employees
SELECT CONCAT('Employee: ', name, ' - Salary: ', salary) FROM employees

-- ============================================
-- 19. MATH FUNCTIONS ON COLUMNS
-- ============================================
SELECT ABS(-42)
SELECT ROUND(3.14159265, 4)
SELECT ROUND(2.5)
SELECT FLOOR(99.99)
SELECT CEIL(0.01)
SELECT MOD(100, 7)

-- ============================================
-- 20. LIKE PATTERNS
-- ============================================
SELECT * FROM employees WHERE name LIKE 'A%'
SELECT * FROM employees WHERE name LIKE 'S%'
SELECT * FROM employees WHERE email LIKE '%@heaven.db'

-- ============================================
-- 21. BETWEEN
-- ============================================
SELECT name, salary FROM employees WHERE salary BETWEEN 50000 AND 80000

-- ============================================
-- 22. IN
-- ============================================
SELECT * FROM employees WHERE id IN (1, 3, 5)
SELECT * FROM projects WHERE status IN ('active', 'planning')

-- ============================================
-- 23. JOINS
-- ============================================
SELECT * FROM employees INNER JOIN assignments ON employees.id = assignments.employee_id
SELECT * FROM employees LEFT JOIN assignments ON employees.id = assignments.employee_id
SELECT * FROM employees RIGHT JOIN assignments ON employees.id = assignments.employee_id
SELECT * FROM employees FULL JOIN assignments ON employees.id = assignments.employee_id
SELECT * FROM employees CROSS JOIN departments

-- ============================================
-- 24. UNION
-- ============================================
SELECT name FROM employees UNION SELECT name FROM departments
SELECT name FROM employees UNION ALL SELECT name FROM projects

-- ============================================
-- 25. UPDATE TESTS
-- ============================================
UPDATE employees SET salary = 90000.00 WHERE id = 1
UPDATE employees SET is_active = FALSE WHERE id = 4
UPDATE employees SET name = 'Aditya SDE' WHERE id = 1
SELECT * FROM employees WHERE id = 1
SELECT * FROM employees WHERE id = 4

-- ============================================
-- 26. DELETE TESTS
-- ============================================
DELETE FROM assignments WHERE id = 5
SELECT * FROM assignments

-- ============================================
-- 27. TRANSACTIONS - STRESS
-- ============================================
BEGIN
INSERT INTO employees VALUES (NULL, 'Temp1', 't1@x.com', 1000.00, TRUE, '{}')
COMMIT
BEGIN
INSERT INTO employees VALUES (NULL, 'Temp2', 't2@x.com', 2000.00, TRUE, '{}')
COMMIT
BEGIN
INSERT INTO employees VALUES (NULL, 'Temp3', 't3@x.com', 3000.00, TRUE, '{}')
ROLLBACK
BEGIN
INSERT INTO employees VALUES (NULL, 'Temp4', 't4@x.com', 4000.00, TRUE, '{}')
COMMIT
SELECT * FROM employees WHERE name LIKE 'Temp%'

-- ============================================
-- 28. VIEWS
-- ============================================
CREATE VIEW active_employees AS SELECT * FROM employees WHERE is_active = TRUE
SELECT * FROM active_employees
CREATE VIEW high_earners AS SELECT name, salary FROM employees WHERE salary > 70000
SELECT * FROM high_earners
DROP VIEW high_earners
DROP VIEW active_employees

-- ============================================
-- 29. ALTER TABLE
-- ============================================
ALTER TABLE employees ADD COLUMN phone TEXT
ALTER TABLE employees ADD COLUMN bonus FLOAT
SELECT * FROM employees
ALTER TABLE employees DROP COLUMN phone
ALTER TABLE employees DROP COLUMN bonus
SELECT * FROM employees

-- ============================================
-- 30. INDEX
-- ============================================
CREATE INDEX idx_emp_salary ON employees(salary)
CREATE INDEX idx_emp_name ON employees(name)

-- ============================================
-- 31. ADMIN
-- ============================================
SHOW TABLES
DESCRIBE employees
DESCRIBE departments
DESCRIBE projects
EXPLAIN SELECT * FROM employees WHERE salary > 50000
EXPLAIN SELECT * FROM employees WHERE is_active = TRUE

-- ============================================
-- 32. BACKUP
-- ============================================
BACKUP TO 'test2_backup.hdb'

-- ============================================
-- 33. FINAL STATE
-- ============================================
SELECT * FROM employees
SELECT COUNT(*) FROM employees
SELECT COUNT(*) FROM departments
SELECT COUNT(*) FROM projects
SELECT COUNT(*) FROM assignments