Associative FS:

 - be content-addressable
   - same object will have same hash and be stored only once
   - changing the object will automagically produce a new version without overwriting the old one

 - there is no file tree
   - instead, there are attribute dictionaries, which point to a particular blob hash

 - there might be a b+tree of blob hashes for faster search


blobs might point back at their metadata?


Global Attribute Dictionary
```
 attr. name | attr. value | hash
------------+-------------+------------
NAME        | aaa.jpg     |  341435234123       (impl detail: hash is a direct pointer into the HashLoc table?)
CREATED     | 2010-10-01  |  341435234123
DOWNLOAD_URL| http://bla. |  341435234123
```

HashLoc table
```
         hash | disk location
--------------+------------------
341435234123  | block 100500
```
