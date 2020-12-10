import org.json.simple.JSONObject;
import org.json.simple.JSONValue;
import org.json.simple.JSONArray;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

//http://commons.apache.org/proper/commons-codec/
import org.apache.commons.codec.binary.Hex;
import org.apache.commons.codec.DecoderException;

import java.util.ArrayList;

class PassList
{
    private ArrayList<String> passList;
    private int listId;


    PassList(String initString)
    {
        try
        {
            JSONParser connectionParser = new JSONParser();
            JSONObject connection = (JSONObject) connectionParser.parse(initString);











            registrationId = ((Long) connection.get("RegistrationId")).intValue();
            functionId = ((Long) connection.get("FunctionId")).intValue();
            hostName  = ((String) connection.get("HostName"));
            hostPort = ((Long) connection.get("HostPort")).intValue();
            String tmpString = (String) connection.get("InitBytes");
            initSequence = new VdmsTransaction( tmpString.length(), Hex.decodeHex(tmpString));
            initSequenceSizeMultiplier = ((Long) connection.get("InitBytesMultiplier")).intValue();
        }  
        catch(ParseException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
        catch(DecoderException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }

    }

    public int GetId()
    {
        return listId;
    }

    public ArrayList<String> GetPassList()
    {
        return passList;
    }

    /*
    Will allow the ability to modify the pass list
    */





}