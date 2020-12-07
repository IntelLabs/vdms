import org.json.simple.JSONObject;
import org.json.simple.JSONValue;
import org.json.simple.JSONArray;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

//http://commons.apache.org/proper/commons-codec/
import org.apache.commons.codec.binary.Hex;
import org.apache.commons.codec.DecoderException;

class VdmsConnection
{
    enum FunctionID { ConfigManager, DataFlowManager, ReplicationPlugin, FilterPlugin, AutoTaskPlugin};

    protected int registrationId;
    protected int functionId;
    protected String hostName;
    protected int hostPort;
    protected byte[] initSequence;

    VdmsConnection()
    {

    }

    VdmsConnection(String initString)
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
            initSequence = Hex.decodeHex(tmpString);
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

    public int GetegistrationRId()
    {
        return registrationId;
    }

    public int GetFunctionId()
    {
        return functionId;
    }

    public String GetHostName()
    {
        return hostName;
    }

    public int GetHostPort()
    {
        return hostPort;
    }

    public byte[] GetInitSequence()
    {
        return initSequence;
    }
    


}