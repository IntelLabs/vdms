import org.json.simple.JSONObject;
import org.json.simple.JSONArray;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

import java.util.ArrayList;

class PassList
{
    private ArrayList<String> passList;
    private int listId;
    private int priority;
    private int matchType; //0 - matching the type of uwry primittive , 1 matching by value of data
    private int unionFlag; /// Union = 1  or Intersection = 0
    private ArrayList<String> criteriaList;


    private JSONObject jsonValue;
    private JSONArray filterCriteria;


    PassList(String initString)
    {
        try
        {
            JSONParser listParser = new JSONParser();
            jsonValue = (JSONObject) listParser.parse(initString);
            JSONObject listValue = jsonValue;
            listId = ((Long) listValue.get("ListId")).intValue();
            priority = ((Long) listValue.get("Priority")).intValue();
            unionFlag = ((Long) listValue.get("Union")).intValue();
            filterCriteria = (JSONArray) listValue.get("Fields");
            for(int i = 0; i < filterCriteria.size(); i++)
            {
                criteriaList.add((String) filterCriteria.get(i));
            }
        }  
        catch(ParseException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }

    public int GetListId()
    {
        return listId;
    }

    public ArrayList<String> GetPassList()
    {
        return passList;
    }

    public String GetValue(int index)
    {
        return passList.get(index);
    }

    public int GetSize()
    {
        return passList.size();
    }

    /*
    Will allow the ability to modify the pass list
    */





}