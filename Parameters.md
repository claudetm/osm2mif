This page describes how to use parameters for the OSM to MIF conversion.

**General Notes**

The Parameters text file is a file that contains instructions for how the conversion should behave.  It can be used to filter only the desired lat/long bounding box, or only the desired features, etc.

The OSM2MIF conversion program does not care in which order you put your parameters.

You can use the C++-style comment "//" at the start of a line to make the program ignore that line.  (This can be useful when you are experimenting).

**Specifying a bounding box**

You can specify the coordinates of a bounding box, so that only nodes and ways within that box are converted.  Do this using the following syntax in the parameters file:

min\_lat="-39.3" max\_lat="-11.0" min\_lon="113.4" max\_lon="153.8"


**Specifying features to include**

You specify the features you want to include by specifying _keys_ and _values_.

The basic example is:
```
k="highway" iv="primary"
```

Here "iv" means "included value".  Specifying k="highway" here means that the value for the highway key in the OSM will be passed through as a column in the output MID file.

Furthermore, keys can be specified as _mandatory_, so that the ways in the OSM have to meet certain criteria in order to be included in the conversion.  For example:

```
mk="highway" iv="primary"
```

In this example it means that only OSM ways which have key-value pairs that include the "highway" key will be included in the conversion.

If multiple mandatory keys are found, an OSM way will be included if there is _at least one_ mandatory key found, e.g.:

```
mk="natural" iv="coastline"
mk="waterway" iv="river"
```

In this example, all OSM ways that have a key of "natural" or "waterway" will be included in the conversion.

You can also use _wildcards_, e.g.

```
mk="highway" iv="*"
```

Here the asterisk indicates that all values found that are associated with highways are to be included (unless specifically excluded with "ev", see below).  Note that you can also use parameters of the form ev="**".**

There is a special value, "id", that can also be used which corresponds to the way's ID, use:

```
k="id" iv="*"
```

This will make the way's numerical ID come out as a column in the MID file.

To exclude ways that have particular key-value pairs, use the "ev" tag like so:

```
mk="highway" iv="*" ev="footway"
```

Here all highway values are included except for "footway".

There are also a number of tags you can use to control more finely the MID and MIF output, which we demonstrate with the following example:

```
mk="waterway" iv="river" style="Pen(3,2,255)" mif_type="Region" break_up="no" tv="River" type="Char(100)"
```

Here we are saying to include OSM ways that have the key-value pair "waterway"-"river", and that:
  * the style used (in the MIF file) for this way should be Pen(3,2,255)
  * the MIF type is Region (not polyline which is the default)
  * there should be no attempt to break up the way at intersections (which is really only relevant for roads and paths)
  * the "river" value should be capitalised in the MID file - "tv" means "transformed value"
  * the MIF type should be Char(100)

**Putting it all together**

The following example should make it clearer how all these parameters interact:

```
mk="highway" iv="*" ev="ferry" ev="ski" ev="bicycle" ev="footway" ev="pedestrian" ev="cycleway" ev="steps" ev="bridleway" ev="escalator" ev="track" ev="path"
k="route" ev="*"
```

In this example, we are trying to pull out the streets from the OSM data that can be used for car or truck driving (that is, the "routable" data).  We specify that the "highway" key is mandatory, so that only ways in the OSM that have "highway" as a key in their key-value pairs will get converted.  We then use iv="