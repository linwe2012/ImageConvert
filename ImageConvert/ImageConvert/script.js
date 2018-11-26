pic = "./madrid_hdr"
im = imread(pic + ".jpg")

mirror = [1,1,0] // mirror flips both x & y axis
imwrite(pic + "flip.png", imflip2d(im, mirror)) 

// move 300px along x & 100px along y
imwrite(pic + "trans.png", imtranslate2d(im, [300, 100]))

scaler = [0.7, 1.01] // shrink width by 0.7 & grow taller by 1.01
imwrite(pic + "scale_large.png", imscale2d(im, scaler))

theta = 0.5
imwrite(pic + "rotate.png", imrotate2d(im, theta))

factor = [0.1, 0.05]
im_shear = imshear2d(im, factor)
imwrite(pic + "shear.png", im_shear)
figure("Image After Shear")
imshow(im_shear)
// it is important to delete image if not using, 
// since it takes a hell lot of memory.
delete im_shear
