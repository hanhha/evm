# Eulerian Video Magnification

This is a proof-of-concept implementation reproducing the work of Hao-Yu Wu,
Michael Rubinstein, Eugene Shih, John Guttag, Fredo Durand, and William T.
Freeman at MIT CSAIL, published in ACM Trans. Graph. (Proceedings SIGGRAPH
2012), titled "Eulerian Video Magnification for Revealing Subtle Changes in
the World.

The basic idea is that given a video of a stationary being, such as a
person, there are minute variations in pixel color, say due to blood flow.
Taking a time series of that pixel (a voxel), band-pass filtering it around
say the expected heart rate, amplifying, and then reconstructing the video,
makes those changes visible to the human eye.
