#include <string>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <fstream>
#include <map>
#include <iostream>
#include <math.h>
#include <set>
#include <vector>
#include <iomanip>
#include <sstream>

using namespace std;


#define LINE_BUFFER_LENGTH 100000

// Read a line from an input file stream
bool GetLineFromFile(ifstream& In, char*& szLine, bool fAllowEmptyLine = false)
{
	static char szBuffer[LINE_BUFFER_LENGTH];
	szLine = NULL;
    if (In.eof ())
        return false;
    In.getline (szBuffer, LINE_BUFFER_LENGTH);
    if (strlen (szBuffer) == LINE_BUFFER_LENGTH || (!fAllowEmptyLine && strlen (szBuffer) == 0))
        return false;
	szLine = szBuffer;
	return true;
}

// Convert a string to a long, returning false if conversion failed
bool ConvertTextTolong(const char* szValue, long& lValue)
{
	char* szEnd;
	errno = 0;
	lValue = strtol(szValue, &szEnd, 10);
	return errno != ERANGE;
}

// Convert a string to a double, returning false if conversion failed
bool ConvertTextToDouble(const char* szValue, double& dblValue)
{
	char* szEnd;
	errno = 0;
	dblValue = strtod(szValue, &szEnd);
	return errno != ERANGE;
}

// This class stores parameter information as specified in the parameters file
class ParameterValues
{
public:
	ParameterValues(bool fIsMandatory = false)
	{
		m_fIsAll = false;
		m_fIsMandatory = fIsMandatory;
	}
	bool m_fIsAll;
	bool m_fIsMandatory;
	set<string> m_setValues;  // the values to be retrieved for this key.  May contain '*' meaning all values.

	// the following maps store style, transformation, mif type etc for each included value string
	map<string, string> m_mapDrawStyle;
	map<string, string> m_mapTransform;
	map<string, string> m_mapMifType;
	map<string, string> m_mapTypes;
	map<string, string> m_mapBreakUp;
};


// Parameters file:
//  * Reads in a file of key/allowed values where the key must match the values to be passed through to the mid/mif (iv = "included values")
//        e.g. k="highway" iv="residential" iv="secondary" iv="primary"
//             or: k="highway" iv="*"    ['*' is a wildcard, so all key value pairs starting with k="highway" will be passed through.  
//											If k="highway" is not in the record at all, it won't be passed through.]
//  * Key/disallowed values: if the key matches one of the values, it is not passed through: (ev = "excluded values")
//        e.g. k="route" ev="ferry" ev="ski" ev="bicycle"
//             or: k="route" ev="*"    ['*' is a wildcard, so all key value pairs starting with k="route" will not be passed through]
//  * Style: Key-included value pairs that, if they match, specify particular pen colour/style/width to use in the mif file
//        e.g. k="waterway" iv="river" style="Pen(3,2,65438)"
//             k="waterway" iv="*" style="Pen(3,2,65438)"    ['*' is a wildcard matching all unmatched values]
//  * Style: Key-included value pairs that, if they match, specify a transformed value to use instead of the original value: (tv = "transformed value")
//        e.g. k="waterway" iv="river" tv="1"
//  * Break up: Whether to break up polylines/regions at intersections - the default is to do this breakup, but it is really only relevant for streets to produce a routable topology.
//        e.g. k="waterway" break_up="no" to switch off breaking up for waterways
//  Note you can't specify k="key" v="*" for both allowed and disallowed where 'key' is the same in both.
//  * Type: for each column (usually Integer or Char(100), Char(250) etc) - use type="..."
//		  e.g. k="oneway" iv="*" type="Integer"
//  * Mif_type: default mif type is PolyLine ("Pline") but rivers, lakes, admin boundaries etc should be regions
//         (Mif types are: point, line, polyline, region, arc, text, rectangle, rounded rectangle, ellipse, multipoint, collection)
//        e.g. k="natural" iv="water" style="Pen(3,2,255)" mif_type="Region" break_up="no"
//
//  Also, specific key values specified are always respected (ie. they override the wildcard).
//
// Possible todo's:
//   * Allow further rectangles specifying excluded regions specified by lat/long rects

bool ReadParametersFile(string strParametersFile, double& min_lon, double& min_lat, double& max_lon, double& max_lat, 
						map<string, ParameterValues*>& mapIncludedValues, map<string, ParameterValues*>& mapExcludedValues, string& strError)
{
	ifstream in;
	in.open(strParametersFile.c_str());
	if (!in.good())
		return false;

	char* s;
	while (GetLineFromFile(in, s, true))
	{
		string str(s);

		string strCurrentKey, strCurrentIncludedValue;
		ParameterValues* CurrentIncludedValues = NULL, * CurrentExcludedValues = NULL;
		bool fFirst = true, fCurrentKeyIsMandatory = false;

		while (!str.empty())
		{
			if (str.length() >= 2 && str.substr(0, 2) == "//")
				break;

			int eq = str.find("=\"");
			int ve = str.find("\"", eq + 2);
			if (eq == str.npos && ve == str.npos)
				break;
			if (eq == str.npos || ve == str.npos)
			{
				strError = "Error in \'" + str + "\'";
				return false;
			}
			string strKey = str.substr(0, eq), strValue = str.substr(eq + 2, ve - eq - 2);
			if (strKey.empty() || strValue.empty())
			{
				strError = "Error in \'" + str + "\'";
				return false;
			}

			if (fFirst)
			{
				strCurrentKey = "";
				CurrentIncludedValues = NULL;
				CurrentExcludedValues = NULL;

				if (strKey == "min_lon")
				{
					if (!ConvertTextToDouble(strValue.c_str(), min_lon))
					{
						strError = "Error in \'" + str + "\': value is not a lat/long";
						return false;
					}
				}
				else if (strKey == "max_lon")
				{
					if (!ConvertTextToDouble(strValue.c_str(), max_lon))
					{
						strError = "Error in \'" + str + "\': value is not a lat/long";
						return false;
					}
				}
				else if (strKey == "min_lat")
				{
					if (!ConvertTextToDouble(strValue.c_str(), min_lat))
					{
						strError = "Error in \'" + str + "\': value is not a lat/long";
						return false;
					}
				}
				else if (strKey == "max_lat")
				{
					if (!ConvertTextToDouble(strValue.c_str(), max_lat))
					{
						strError = "Error in \'" + str + "\': value is not a lat/long";
						return false;
					}
				}
				else if (strKey == "k" || strKey == "mk")
				{
					strCurrentKey = strValue;
					fCurrentKeyIsMandatory = (strKey == "mk");
				}
				else
				{
					strError = "Error in \'" + str + "\': unrecognised key";
					return false;
				}
			}
			else
			{
				if (strKey == "iv")
				{
					if (CurrentIncludedValues == NULL)
					{
						if (mapIncludedValues.find(strCurrentKey) != mapIncludedValues.end())
							CurrentIncludedValues = mapIncludedValues.find(strCurrentKey)->second;
						else
						{
							CurrentIncludedValues = new ParameterValues(fCurrentKeyIsMandatory);
							mapIncludedValues[strCurrentKey] = CurrentIncludedValues;
						}
					}
					if (strValue == "*")
						CurrentIncludedValues->m_fIsAll = true;
					else
						CurrentIncludedValues->m_setValues.insert(strValue);
					strCurrentIncludedValue = strValue;
				}
				if (strKey == "ev")
				{
					if (CurrentExcludedValues == NULL)
					{
						if (mapExcludedValues.find(strCurrentKey) != mapExcludedValues.end())
							CurrentExcludedValues = mapExcludedValues.find(strCurrentKey)->second;
						else
						{
							CurrentExcludedValues = new ParameterValues;
							mapExcludedValues[strCurrentKey] = CurrentExcludedValues;
						}
					}
					if (strValue == "*")
						CurrentExcludedValues->m_fIsAll = true;
					else
						CurrentExcludedValues->m_setValues.insert(strValue);
				}
				if (strKey == "style")
					CurrentIncludedValues->m_mapDrawStyle[strCurrentIncludedValue] = strValue;
				if (strKey == "tv")
					CurrentIncludedValues->m_mapTransform[strCurrentIncludedValue] = strValue;
				if (strKey == "mif_type")
					CurrentIncludedValues->m_mapMifType[strCurrentIncludedValue] = strValue;
				if (strKey == "type")
				{
					if (CurrentIncludedValues->m_mapTypes.find(strCurrentKey) != CurrentIncludedValues->m_mapTypes.end())
					{
						strError = "Error in \'" + str + "\': type defined twice for " + strCurrentKey;
						return false;
					}
					CurrentIncludedValues->m_mapTypes[strCurrentKey] = strValue;
				}
				if (strKey == "break_up" && strValue == "no")
					CurrentIncludedValues->m_mapBreakUp[strCurrentKey] = strValue;
			}

			if (ve + 2 >= (int)str.length())
				break;
			else
				str = str.substr(ve + 2);
			fFirst = false;
		}
	}

	in.close();
	return true;
}

// class to store relation data
class Relation
{
public:
	Relation()
	{
		m_to_way_id = -1;
		m_node_via_id = -1;
		m_fIsRestriction = false;
	}
	vector<long> m_from_way_ids;
	long m_to_way_id, m_node_via_id;
	bool m_fIsRestriction;
};

typedef pair<multimap<long,Relation*>::iterator, multimap<long,Relation*>::iterator> RelationsItPair;

// XML parsing helper function
string ReplaceApostrophesAndAmpersands(string str)
{
	size_t nFind = 0;
	string strFind = "&apos;", strReplace = "'";
	for (; (nFind = str.find(strFind, nFind)) != string::npos; )
	{
		str.replace(nFind, strFind.length(), strReplace);
		nFind += strReplace.length();
	}
	nFind = 0;
	strFind = "&amp;";
	strReplace = "&";
	for (; (nFind = str.find(strFind, nFind)) != string::npos; )
	{
		str.replace(nFind, strFind.length(), strReplace);
		nFind += strReplace.length();
	}
	return str;
}

void WriteMidMifRecord(ofstream& outMid, ofstream& outMif, const string& strMifTypeForThisWay, const string& strStyleForThisWay, 
					   vector<pair<double,double> >& latlons, map<string, string>& values_in_current_way, bool fWriteRelations, const string& strRelationData)
{
	if (strMifTypeForThisWay == "Region" || strMifTypeForThisWay == "region")
		outMif << "Region 1" << endl << "  " << latlons.size() << endl;
	else
		outMif << "Pline " << latlons.size() << endl;

	for (vector<pair<double,double> >::iterator itLatLon = latlons.begin(); itLatLon != latlons.end(); itLatLon++)
		outMif << setprecision(15) << itLatLon->second << " " << itLatLon->first << endl;

	outMif << "	" << strStyleForThisWay << endl;

	int j = 0;
	for (map<string, string>::iterator itValue = values_in_current_way.begin(); itValue != values_in_current_way.end(); itValue++, j++)
	{
		if (j > 0)
			outMid << ",";
		outMid << "\"" << ReplaceApostrophesAndAmpersands(itValue->second) << "\"";
	}

	if (fWriteRelations)
		outMid << "," << strRelationData;
	outMid << endl;
}

double AngleBetweenIntersectingLines(double dblLine1XFrom, double dblLine1YFrom, double dblLine1XTo, double dblLine1YTo, 
									 double dblLine2XFrom, double dblLine2YFrom, double dblLine2XTo, double dblLine2YTo)
{
	// create vectors (delta x and delta y) out of the lines
	double dblX1 = dblLine1XTo - dblLine1XFrom, dblY1 = dblLine1YTo - dblLine1YFrom;
	double dblX2 = dblLine2XTo - dblLine2XFrom, dblY2 = dblLine2YTo - dblLine2YFrom;

	// now calc angles
	double angle1 = atan2(dblY1, dblX1);		// Angle made with the horizontal
	double angle2 = atan2(dblY2, dblX2);		// Angle made with the horizontal
	const double radians_to_degrees = 57.29577951289617186797;
	double degrees = radians_to_degrees*(angle2 - angle1);		// Angle between lines

	// Convert to lie interval [-360,360]
	int sign = (degrees < 0 ? -1 : (degrees == 0 ? 0 : 1));
	degrees = fabs(degrees);
	degrees = (degrees - 360*( ((long)degrees)/((long)360) ));

	return sign * (degrees <= 180.0 ? degrees : (degrees - 360.0));
}

bool IsRightTurn(double dblLine1XFrom, double dblLine1YFrom, double dblLine1XTo, double dblLine1YTo, 
				 double dblLine2XFrom, double dblLine2YFrom, double dblLine2XTo, double dblLine2YTo)
{
	return AngleBetweenIntersectingLines(dblLine1XFrom, dblLine1YFrom, dblLine1XTo, dblLine1YTo, 
		 								 dblLine2XFrom, dblLine2YFrom, dblLine2XTo, dblLine2YTo) < 0;
}

// Function to try and pull out banned right turn
string GetRelationData(RelationsItPair& itRelations, map<long, vector<long> >& nodes_in_each_way, 
					   long id_of_from_way, int nUptoNodeInFromWay,
					   map<long, double>& latitudes, map<long, double>& longitudes,
					   int& nRelationsWritten, int& nRelationsFound, bool fLookAtNextNodeInWayToDetermineIfIsRightTurn)
{
	if (nUptoNodeInFromWay < 0)
		return "";

	stringstream str;

	for (multimap<long,Relation*>::iterator itRel = itRelations.first; itRel != itRelations.second; itRel++)
	{
		long node_id = nodes_in_each_way[id_of_from_way][nUptoNodeInFromWay];

		if (itRel->second->m_node_via_id == node_id)
		{
			nRelationsFound++;

			// we need to find the next or previous node in the 'to' way
			long from_node_id_in_to_way = -1, to_node_id_in_to_way = -1;
			for (vector<long>::iterator it = nodes_in_each_way[itRel->second->m_to_way_id].begin(); it != nodes_in_each_way[itRel->second->m_to_way_id].end(); it++)
				if (*it == itRel->second->m_node_via_id && it != nodes_in_each_way[itRel->second->m_to_way_id].end() - 1)
				{
					from_node_id_in_to_way = *it;
					to_node_id_in_to_way = *(++it);
					break;
				}
				else if (*it == node_id && it != nodes_in_each_way[itRel->second->m_to_way_id].begin())
				{
					from_node_id_in_to_way = *it;
					to_node_id_in_to_way = *(--it);
					break;
				}

			if (from_node_id_in_to_way >= 0 && to_node_id_in_to_way >= 0)
			{
				long prev_node_id = nodes_in_each_way[id_of_from_way][nUptoNodeInFromWay - 1];

				if (!fLookAtNextNodeInWayToDetermineIfIsRightTurn 
					&&
					IsRightTurn(longitudes[prev_node_id], latitudes[prev_node_id],
								longitudes[node_id], latitudes[node_id],
								longitudes[from_node_id_in_to_way], latitudes[from_node_id_in_to_way],
								longitudes[to_node_id_in_to_way], latitudes[to_node_id_in_to_way]))
				{
					str << (str.str().length() > 1 ? ";" : "") << itRel->second->m_to_way_id;
					nRelationsWritten++;
				}
				else if (fLookAtNextNodeInWayToDetermineIfIsRightTurn 
						&& 
						nUptoNodeInFromWay < (int)nodes_in_each_way[id_of_from_way].size() - 1)
				{
					int next_node_id = nodes_in_each_way[id_of_from_way][nUptoNodeInFromWay + 1];

					if (IsRightTurn(longitudes[next_node_id], latitudes[next_node_id],
									longitudes[node_id], latitudes[node_id],
									longitudes[from_node_id_in_to_way], latitudes[from_node_id_in_to_way],
									longitudes[to_node_id_in_to_way], latitudes[to_node_id_in_to_way]))
					{
						str << (str.str().length() > 1 ? ";" : "") << itRel->second->m_to_way_id;
						nRelationsWritten++;
					}
				}
			}
		}
	}

	return str.str();
}

void ReadKeyValuePairsForWay(char*& s, map<string, ParameterValues*>& mapIncludedValues, map<string, ParameterValues*>& mapExcludedValues,
							map<string, string>& values_in_current_way, string& strMifTypeForThisWay, string& strStyleForThisWay,
							bool& fBreakUpThisWay, bool& fSkipThisWay, bool& fFoundAtLeastOneIncludedValueInThisWay,
							int& nNumberOfMandatoryKeysFoundForThisWay)
{
	fBreakUpThisWay = false;

	char* tag = strstr(s, "<tag k=\""), * tag_end = (tag != NULL ? strstr(tag + 8, "\"") : NULL);
	if (tag != NULL && tag_end != NULL)
	{
		char* v = strstr(tag, "v=\""), * v_end = (v != NULL ? strstr(v + 3, "\"") : NULL);
		if (v != NULL && v_end != NULL)
		{
			string strFind(tag + 8, tag_end);
			string strValue(v + 3, v_end);

			map<string, ParameterValues*>::iterator itKeyExclude = mapExcludedValues.find(strFind);
			if (itKeyExclude != mapExcludedValues.end() 
				&& 
				(itKeyExclude->second->m_fIsAll || itKeyExclude->second->m_setValues.find(strValue) != itKeyExclude->second->m_setValues.end()))
					fSkipThisWay = true;
			else
			{
				map<string, ParameterValues*>::iterator itKey = mapIncludedValues.find(strFind);
				if (itKey != mapIncludedValues.end())
				{
					if (!strValue.empty()
						&&
						(itKey->second->m_fIsAll || itKey->second->m_setValues.find(strValue) != itKey->second->m_setValues.end()))
					{
						fFoundAtLeastOneIncludedValueInThisWay = true;
						if (itKey->second->m_fIsMandatory)
							nNumberOfMandatoryKeysFoundForThisWay++;

						string strTransformedValue = strValue;
						if (itKey->second->m_mapTransform.find(strValue) != itKey->second->m_mapTransform.end())
							strTransformedValue = itKey->second->m_mapTransform.find(strValue)->second;
						values_in_current_way[itKey->first] = strTransformedValue;

						if (itKey->second->m_mapDrawStyle.find(strValue) != itKey->second->m_mapDrawStyle.end())
							strStyleForThisWay = itKey->second->m_mapDrawStyle.find(strValue)->second;

						if (itKey->second->m_mapMifType.find(strValue) != itKey->second->m_mapMifType.end())
							strMifTypeForThisWay = itKey->second->m_mapMifType.find(strValue)->second;

						if (itKey->second->m_mapBreakUp.find("no") != itKey->second->m_mapBreakUp.end())
							fBreakUpThisWay = false;
					}
				}
			}
		}
	}
}


int main(int argc, char* argv[])
{
	if (argc < 4 || argc > 5)
	{
		cout << "Usage: OSM2MIF  OSM_input_file_name  Parameters_file  MIF_output_file_name  [-no_relations]" << endl;
		exit(0);
	}

	string strInFile = argv[1];
	string strParameterFile = argv[2];
	string strOutFileMid = argv[3] + string(".mid"), strOutFileMif = argv[3] + string(".mif");

	bool fProcessRelations = true;
	if (argc == 5 && string(argv[4]) == "-no_relations")
		fProcessRelations = false;

	double min_lon = LONG_MAX, min_lat = LONG_MAX, max_lon = LONG_MAX, max_lat = LONG_MAX;

	map<string, ParameterValues*> mapIncludedValues, mapExcludedValues;
	string strError;

	if (!ReadParametersFile(strParameterFile, min_lon, min_lat, max_lon, max_lat, mapIncludedValues, mapExcludedValues, strError))
	{
		cout << "Error in Parameters File: " << strError << endl;
		exit(0);
	}

	int node_count = 0, nodes_skipped = 0, way_count = 0, ways_skipped = 0, ways_written = 0;

	ifstream in;
	in.open(strInFile.c_str());
	if (!in.good ())
	{
		cout << "Could not open " << strInFile << " for reading" << endl;
		return 0;
	}

	ofstream outMid, outMif;
	outMid.open(strOutFileMid.c_str(), ios::trunc);
	outMif.open(strOutFileMif.c_str(), ios::trunc);
	if (!outMid.good())
	{
		cout << "Could not open " << strOutFileMid << " for writing" << endl;
		return 0;
	}
	if (!outMif.good())
	{
		cout << "Could not open " << strOutFileMif << " for writing" << endl;
		return 0;
	}

	outMif << "Version 300" << endl;
	outMif << "Charset \"Neutral\"" << endl;
	outMif << "Delimiter \",\"" << endl;
	outMif << "CoordSys Earth Projection 1, 74 Bounds (-1000, -1000) (1000, 1000)" << endl;
	outMif << "Columns " << mapIncludedValues.size() + 1 << endl;
	for (map<string, ParameterValues*>::iterator itKey = mapIncludedValues.begin(); itKey != mapIncludedValues.end(); itKey++)
	{
		outMif << "    " << itKey->first;
		map<string, string>::iterator itType = itKey->second->m_mapTypes.find(itKey->first);
		if (itType != itKey->second->m_mapTypes.end())
			outMif << " " << itType->second << endl;
		else
			outMif << " Char(250)" << endl;
	}
	if (fProcessRelations)
		outMif << "    Restrictions Char(250)" << endl;
	outMif << "Data" << endl;

	map<long, double> latitudes;
	map<long, double> longitudes;
	map<long, int> way_counts;
	map<long, vector<long> > nodes_in_each_way;
	multimap<long, Relation*> relations;

	int line;
	long id_of_current_way = LONG_MAX;
	Relation* current_relation = NULL;

	// We read in the OSM file twice:
	//     The first time, we store the lat/long data for each node (in the bounding box); the nodes in each way; the 
	//       number of times a node appears in the ways ("way_counts").  We also store any relations found.
	//     The second time we read the file, we only read the ways, and we output them to mid/mif.

	for (line = 0; line < INT_MAX; line++)
	{
		char* s;
		if (!GetLineFromFile(in, s))
		{
			printf("Read %d lines\n", line);
			return 0;
		}

		if (strstr(s, "</osm>") != NULL)
			break;

		if (strstr(s, "<node id=") != NULL)
		{
			current_relation = NULL;

			char* id, * id_end, * lat, * lon, * lat_end, * lon_end;
			id = strstr(s, "<node id=\"");
			id_end = (id != NULL ? strstr(id+strlen("<node id=\"")+1, "\"") : NULL);
			lat = strstr(s, "lat=\"");			
			lon = strstr(s, "lon=\"");			
			lat_end = (lat != NULL ? strstr(lat+5, "\"") : NULL);
			lon_end = (lon != NULL ? strstr(lon+5, "\"") : NULL);
			
			if (id != NULL && id_end != NULL && lat != NULL && lon != NULL && lat_end != NULL && lon_end != NULL)
			{
				long node_id;
				if (!ConvertTextTolong(id+strlen("<node id=\""), node_id))
				{
					cout << "Could not turn " << id+strlen("<node id=\"") << " into a numeric ID" << endl;
					return 0;
				}
				double latitude, longitude;
				if (!ConvertTextToDouble(lat+5, latitude))
				{
					cout << "Could not turn " << latitude << " into a latitude" << endl;
					return 0;
				}
				if (!ConvertTextToDouble(lon+5, longitude))
				{
					cout << "Could not turn " << longitude << " into a longitude" << endl;
					return 0;
				}

				if (min_lon == LONG_MAX && min_lat == LONG_MAX && max_lon == LONG_MAX && max_lat == LONG_MAX
					||
					longitude >= min_lon && longitude <= max_lon && latitude >= min_lat && latitude <= max_lat)
				{
					latitudes[node_id] = latitude;
					longitudes[node_id] = longitude;
					way_counts[node_id] = 0;
				}
				else
					nodes_skipped++;

				++node_count;
				if (node_count <= 1000000 && node_count % 100000 == 0 || node_count % 1000000 == 0)
					printf("---- Node %d [%d within lat/long bounding box]\n", node_count, latitudes.size());
			}
		}

		if (strstr(s, "<way id=") != NULL)
		{
			id_of_current_way = LONG_MAX;
			char* id = strstr(s, "<way id=\""), * id_end = (id != NULL ? strstr(id+9, "\"") : NULL);
			if (id != NULL && id_end != NULL && !ConvertTextTolong(id+9, id_of_current_way))
			{
				cout << "Could not turn way ID " << id+9 << " into a numeric ID" << endl;
				return 0;
			}
		}

		if (id_of_current_way != LONG_MAX)
		{
			char* id = strstr(s, "<nd ref=\""), * id_end = (id != NULL ? strstr(id+9, "\"") : NULL);
			if (id != NULL && id_end != NULL)
			{
				long node_id;
				if (!ConvertTextTolong(id+9, node_id))
				{
					cout << "Could not turn node ID " << id+9 << " into a numeric ID" << endl;
					return 0;
				}
				way_counts[node_id]++;

				nodes_in_each_way[id_of_current_way].push_back(node_id);
			}
			if (strstr(s, "</way>") != NULL)
				id_of_current_way = LONG_MAX;
		}

		if (strstr(s, "<relation id=") != NULL && fProcessRelations)
			current_relation = new Relation;

		if (current_relation != NULL)
		{
			bool fIsNode = false, fIsWay = false;
			if (strstr(s, "<member type=\"node") != NULL)
				fIsNode = true;
			if (strstr(s, "<member type=\"way") != NULL)
				fIsWay = true;

			if (fIsNode || fIsWay)
			{
				long id_of_member_type = -1;
				char* id = strstr(s, "ref=\""), * id_end = (id != NULL ? strstr(id+5, "\"") : NULL);
				if (id != NULL && id_end != NULL && !ConvertTextTolong(id+5, id_of_member_type))
				{
					cout << "Could not turn relation member ID " << id+5 << " into a numeric ID" << endl;
					return 0;
				}
				bool fIsFromWay = false;
				if (fIsNode && strstr(s, "role=\"via") != NULL)
					;
				else if (fIsWay && strstr(s, "role=\"from") != NULL)
					fIsFromWay = true;
				else if (fIsWay && strstr(s, "role=\"to") != NULL)
					fIsFromWay = false;
				else
					current_relation->m_fIsRestriction = false; // not a restriction

				if (fIsNode)
					current_relation->m_node_via_id = id_of_member_type;
				else if (fIsFromWay)
					current_relation->m_from_way_ids.push_back(id_of_member_type);
				else
					current_relation->m_to_way_id = id_of_member_type;
			}

			if (strstr(s, "<tag k=\"type") != NULL)
			{
				char* v = strstr(s, "v=\""), * v_end = (v != NULL ? strstr(v+3, "\"") : NULL);
				if (v != NULL && v_end != NULL && string(v+3, v_end) == "restriction")
					current_relation->m_fIsRestriction = true;
			}

			if (strstr(s, "</relation>") != NULL && current_relation != NULL)
			{
				if (current_relation->m_from_way_ids.size() > 0 && current_relation->m_to_way_id >= 0 && current_relation->m_node_via_id >= 0)
				{
					for (vector<long>::iterator it = current_relation->m_from_way_ids.begin(); it != current_relation->m_from_way_ids.end(); it++)
						relations.insert(pair<long, Relation*>(*it, current_relation));
				}
				current_relation = NULL;
			}
		}
	}

	// close and reopen, ready to read the ways
	in.close();
	in.open(strInFile.c_str());
	if (!in.good ())
	{
		cout << "Could not open " << strInFile << " for reading" << endl;
		return 0;
	}

	string strDefaultStyle = "Pen (2,54,32768)";
	string strDefaultMifType = "Pline";

	id_of_current_way = LONG_MAX;
	map<string, string> values_in_current_way;
	string strMifTypeForThisWay, strStyleForThisWay;
	bool fBreakUpThisWay = true, fSkipThisWay = false, fFoundAtLeastOneIncludedValueInThisWay = false;
	int nNumberOfMandatoryKeysFoundForThisWay = 0;
	int nRestrictionsWrittenCount = 0, nRestrictionsInWaysCount = 0;

	for (line = 0; line < INT_MAX; line++)
	{
		char* s;
		if (!GetLineFromFile(in, s))
		{
			printf("Read %d lines\n", line);
			return 0;
		}

		if (strstr(s, "</osm>") != NULL)
			break;

		if (strstr(s, "<way id=") != NULL)
		{
			fSkipThisWay = false;
			fFoundAtLeastOneIncludedValueInThisWay = false;
			nNumberOfMandatoryKeysFoundForThisWay = 0;
			strStyleForThisWay = strDefaultStyle;
			strMifTypeForThisWay = strDefaultMifType;
			id_of_current_way = LONG_MAX;

			char* id = strstr(s, "<way id=\""), * id_end = (id != NULL ? strstr(id + 9, "\"") : NULL);
			if (id != NULL && id_end != NULL)
			{
				if (!ConvertTextTolong(id + 9, id_of_current_way))
				{
					cout << "Could not turn way ID " << id + 9 << " into a numeric ID" << endl;
					return 0;
				}

				for (map<string, ParameterValues*>::iterator itKey = mapIncludedValues.begin(); itKey != mapIncludedValues.end(); itKey++)
					values_in_current_way[itKey->first] = (itKey->first == "id" ? string(id + 9, id_end) : "");

				++way_count;
				if (way_count <= 1000000 && way_count % 100000 == 0 || way_count % 100000 == 0)
					printf("---- Way %d [%d written]\n", way_count, ways_written);
			}
		}
		if (id_of_current_way != LONG_MAX)
		{
			ReadKeyValuePairsForWay(s, mapIncludedValues, mapExcludedValues, values_in_current_way, strMifTypeForThisWay, strStyleForThisWay,
									fBreakUpThisWay, fSkipThisWay, fFoundAtLeastOneIncludedValueInThisWay, nNumberOfMandatoryKeysFoundForThisWay);

			if (strstr(s, "</way>") != NULL)
			{
				if (!fSkipThisWay && fFoundAtLeastOneIncludedValueInThisWay && nNumberOfMandatoryKeysFoundForThisWay >= 1)
				{
					bool fWaysWritten = false;

					if (nodes_in_each_way[id_of_current_way].size() > 1)
					{
						vector<pair<double,double> > latlons;

						RelationsItPair itRelations = relations.equal_range(id_of_current_way);

						int i = 0, prev_intersection_i = -1;
						for (vector<long>::iterator it = nodes_in_each_way[id_of_current_way].begin(); it != nodes_in_each_way[id_of_current_way].end(); it++, i++)
						{
							map<long, double>::iterator itFindLat, itFindLon;
							if ( (itFindLat = latitudes.find(*it)) != latitudes.end() && (itFindLon = longitudes.find(*it)) != longitudes.end())
							{
								latlons.push_back(pair<double,double>(itFindLat->second, itFindLon->second));

								if (i > 0 && (i == nodes_in_each_way[id_of_current_way].size() - 1 || fBreakUpThisWay && way_counts[*it] > 1) && latlons.size() > 1)
								{
									// this is the last node of the way, or this node represents an intersection (if we are breaking up ways)
									ways_written++;
									fWaysWritten = true;

									WriteMidMifRecord(outMid, outMif, strMifTypeForThisWay, strStyleForThisWay, latlons, 
														values_in_current_way, fProcessRelations,
													  "\"" + GetRelationData(itRelations, nodes_in_each_way, id_of_current_way, i, latitudes, longitudes, 
																	  nRestrictionsWrittenCount, nRestrictionsInWaysCount, false)
													  + GetRelationData(itRelations, nodes_in_each_way, id_of_current_way, prev_intersection_i, latitudes, longitudes, 
																	  nRestrictionsWrittenCount, nRestrictionsInWaysCount, true) + "\"");

									latlons.erase(latlons.begin(), latlons.end() - 1);
									prev_intersection_i = i;
								}
							}
						}
					}

					if (!fWaysWritten)
						ways_skipped++;
				}

				id_of_current_way = LONG_MAX;
				values_in_current_way.clear();
			}
		}

		// flushing is important to keep peak memory usage low (otherwise the streams consume lots of memory)
		if (line % 10000 == 0)
		{
			outMid.flush();
			outMif.flush();
		}
	}

	in.close();
	outMid.close();
	outMif.close();

	cout << "Processed " << line << " lines from osm file" << endl;
	cout << nodes_skipped << " nodes were skipped" << endl;
	cout << node_count - nodes_skipped << " nodes were read" << endl;
	cout << ways_skipped << " ways were skipped" << endl;
	cout << ways_written << " ways were written" << endl;
	if (fProcessRelations)
	{
		cout << relations.size() << " ways with restriction relations found" << endl;
		cout << nRestrictionsInWaysCount << " restrictions with nodes found" << endl;
		cout << nRestrictionsWrittenCount << " restriction relations written" << endl;
	}
	cout << "Press Enter to exit..." << endl;
	cin.get();

	return 0;
}

