# rectangle

This is used to set the coordinates for a bounding box.
A rectangle has the following form:
```JSON
"rectangle": {
    "x": value,
    "y": value,
    "w": value,
    "h": value
}
```

where `x` and `y` are the coordinates of the *upper left* corner of the
region of interest and `w` and `h` are the width and height (respectively) of the region.
<br>


## Example
```JSON
"rectangle": {
    "x": 100,
    "y": 200,
    "w": 1024,
    "h": 640,
}
```
