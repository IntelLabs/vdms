import vdms

db = vdms.vdms()
db.connect("localhost", 55555)
image_file = open("images/intel_logo.png", "rb")
image_blob = image_file.read()
addImage = {}
addImage["format"] = "png"
props = {}
props["name"] = "intel_logo.png"
addImage["properties"] = props
operations = []
operation = {}
operation["type"] = "custom"
operation["custom_function_type"] = "hsv_threshold"
operation["h0"] = 10
operation["s0"] = 250
operation["v0"] = 175
operation["h1"] = 20
operation["s1"] = 255
operation["v1"] = 185


operations.append(operation)
addImage["operations"] = operations
query = {}
query["AddImage"] = addImage
print(query)
res, blobs = db.query([query], [image_blob])
print(res)
