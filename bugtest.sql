-- ============================================
-- HeavenDB Bug Verification Tests
-- Expected results are written as comments.
-- ============================================

-- Clean slate
DROP TABLE IF EXISTS ftest
DROP TABLE IF EXISTS btest

-- ============================================
-- TEST 1: WHERE on FLOAT column
-- ============================================
CREATE TABLE ftest (id INTEGER, price FLOAT)
INSERT INTO ftest VALUES (1, 10.5)
INSERT INTO ftest VALUES (2, 20.0)
INSERT INTO ftest VALUES (3, 30.5)

-- EXPECT: 1 row (id=2, price=20.0)
-- BUG IF: (0 rows) or wrong row
SELECT * FROM ftest WHERE price > 15

-- EXPECT: 2 rows (id=1, id=3)
SELECT * FROM ftest WHERE price < 25

-- EXPECT: 1 row (id=3)
SELECT * FROM ftest WHERE price = 30.5

-- ============================================
-- TEST 2: WHERE on BOOLEAN column
-- ============================================
CREATE TABLE btest (id INTEGER, active BOOLEAN)
INSERT INTO btest VALUES (1, TRUE)
INSERT INTO btest VALUES (2, FALSE)
INSERT INTO btest VALUES (3, TRUE)

-- EXPECT: 2 rows (id=1, id=3)
SELECT * FROM btest WHERE active = TRUE

-- EXPECT: 1 row (id=2)
SELECT * FROM btest WHERE active = FALSE

-- ============================================
-- TEST 3: ORDER BY does not mutate table
-- ============================================
CREATE TABLE ftest (id INTEGER, price FLOAT)
INSERT INTO ftest VALUES (1, 10.5)
INSERT INTO ftest VALUES (2, 20.0)
INSERT INTO ftest VALUES (3, 30.5)

-- EXPECT: rows in order 3, 2, 1 (by price descending)
SELECT id, price FROM ftest ORDER BY price DESC

-- EXPECT: original insertion order 1, 2, 3
-- BUG IF: order is 3, 2, 1 (mutated)
SELECT id, price FROM ftest

-- ============================================
-- TEST 4: DELETE on FLOAT/BOOLEAN columns
-- ============================================
DELETE FROM ftest WHERE price > 15

-- EXPECT: only id=1 remains
-- BUG IF: all 3 rows still there
SELECT * FROM ftest

-- ============================================
-- TEST 5: UPDATE on FLOAT/BOOLEAN columns
-- ============================================
UPDATE btest SET active = FALSE WHERE id = 1

-- EXPECT: id=1 is FALSE, id=2 is FALSE, id=3 is TRUE
SELECT * FROM btest

-- ============================================
-- TEST 6: Compound WHERE with AND
-- ============================================
CREATE TABLE ftest (id INTEGER, price FLOAT)
INSERT INTO ftest VALUES (1, 10.5)
INSERT INTO ftest VALUES (2, 20.0)
INSERT INTO ftest VALUES (3, 30.5)

-- EXPECT: 1 row (id=3)
SELECT * FROM ftest WHERE price > 15 AND id > 2

-- EXPECT: 2 rows (id=2, id=3)
SELECT * FROM ftest WHERE price > 15 OR id = 1

-- ============================================
-- TEST 8: Compound WHERE with 3+ conditions
-- ============================================
DROP TABLE IF EXISTS wtest
CREATE TABLE wtest (id INTEGER, price FLOAT, active BOOLEAN, name TEXT)
INSERT INTO wtest VALUES (1, 10.5, TRUE, 'alice')
INSERT INTO wtest VALUES (2, 20.0, FALSE, 'bob')
INSERT INTO wtest VALUES (3, 30.5, TRUE, 'carol')
INSERT INTO wtest VALUES (4, 40.0, TRUE, 'dave')
INSERT INTO wtest VALUES (5, 50.0, FALSE, 'eve')

-- EXPECT: 2 rows (id=3, id=4)
SELECT * FROM wtest WHERE price > 15 AND active = TRUE

-- EXPECT: 2 rows (id=4 only; id=3 fails id > 3)
SELECT * FROM wtest WHERE price > 15 AND active = TRUE AND id > 3

-- EXPECT: 3 rows (id=3, id=4, id=5)
SELECT * FROM wtest WHERE price > 35 OR active = FALSE

-- EXPECT: 3 rows (id=2, id=3, id=4) - AND binds tighter than OR
SELECT * FROM wtest WHERE price > 25 AND active = TRUE OR id = 2

-- EXPECT: 1 row (id=4) - parenthesized, different from above
SELECT * FROM wtest WHERE price > 25 AND (active = TRUE OR id = 2)