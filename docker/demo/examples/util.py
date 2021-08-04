import os.path

def display_images(imgs):
    from IPython.display import Image, display
    counter = 0
    for im in imgs:
        img_file = 'images/results_' + str(counter) + '.jpg'
        counter = counter + 1
        fd = open(img_file, 'wb+')
        fd.write(im)
        fd.close()
        display(Image(img_file))
