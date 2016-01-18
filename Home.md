**What does this program do?**

  * Reads in an OSM file, and a parameters file.
  * Outputs a MID and a MIF file.
  * Creates routable data from the OSM - ie. breaks up the OSM ways at intersection nodes, and also processes "no right turn" restrictions.
  * Can flexibly use various filters to control what features (both spatial and data features) get converted.

**Limitations and possible further enhancements**

  * Make it less memory hungry.  Hasn't been tested on anything larger than australia-oceania.osm so far (a 1.8GB file).
  * Be able to specify more than one lat/long bounding box, or a lat/long region - this will better enable filtering out just one country out of a larger OSM file (e.g. extracting and converting just France out of Europe).
  * Be able to handle "no\_left\_turn" restrictions - currently it just handles "no\_right\_turn" restrictions.
  * Make the OSM XML reading and error handling more robust.  Currently makes some mild assumptions about case sensitivity and whitespace etc.

**To compile and build:**

_Windows users_: use the Visual Studio .vcproj file in the repository.

_Linux users_: just compile with "g++ OSM2MIF.cpp -o OSM2MIF".


**To run OSM2MIF - from the command line:**

OSM2MIF  OSM\_input\_file\_name  Parameters\_file  MIF\_output\_file\_name  -no\_relations

(The last parameter is optional).

  * _OSM\_input\_file\_name_ - an OSM file, e.g. australia-oceania.osm, newzealand.osm, etc

  * _Parameters\_file_ - a text file containing parameters (see example below and the [Parameters](Parameters.md) page).

  * _MIF\_output\_file\_name_ - the desired output file name (don't use extensions), e.g. "output".

**How fast is the program?**

Converting the 142MB newzealand.osm file takes around 1 minute on a reasonably fast machine.  Converting the 1.8GB australia-oceania.osm file, filtering for mainland Australia, takes around 10 minutes, and peak memory usage is around 1.5GB of RAM.

For larger files, you may hit memory limits.

**_An example parameters file_** - you would use something like this to get the road features from the australia-oceania OSM file, filtering for mainland Australia, ignoring all footways, cycleways, ferry routes, etc:
```
// Australia:
min_lat="-39.3"
max_lat="-11.0"
min_lon="113.4"
max_lon="153.8"

k="id" iv="*"
k="name" iv="*"
mk="highway" iv="*"
mk="highway" iv="residential" iv="secondary" iv="primary" ev="ferry" ev="ski" ev="bicycle"
mk="highway" ev="footway" ev="pedestrian" ev="cycleway" ev="steps" ev="bridleway" ev="escalator" ev="track" ev="path"
//mk="natural" iv="water" style="Pen(3,2,255)" mif_type="Region" break_up="no"
k="oneway" iv="yes" tv="1" type="Integer"
k="maxspeed" iv="*"
k="route" ev="*"
```

In this example, the columns you would get in the MID file would be "id", "name", "highway", "oneway", "maxspeed" (and possibly also a "restrictions" column depending on how you run the program).