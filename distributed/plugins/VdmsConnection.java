import org.json.simple.JSONObject;
import org.json.simple.JSONValue;
import org.json.simple.JSONArray;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

//http://commons.apache.org/proper/commons-codec/
import org.apache.commons.codec.binary.Hex;
import org.apache.commons.codec.DecoderException;

public abstract class VdmsConnection
{
    enum FunctionID { ConfigManager, DataFlowManager, ReplicationPlugin, FilterPlugin, AutoTaskPlugin};
   
    protected JSONObject jsonValue;
    protected int registrationId;
    protected int priority;
    protected int functionId;
    protected String hostName;
    protected int hostPort;
    protected VdmsTransaction initSequence;
    protected int initSequenceSizeMultiplier;
    protected int passListId;
    
    VdmsConnection()
    {
        
    }
    
    VdmsConnection(String initString)
    {
        try
        {
            JSONParser connectionParser = new JSONParser();
            jsonValue = (JSONObject) connectionParser.parse(initString);
            JSONObject connection = jsonValue;
            registrationId = ((Long) connection.get("RegistrationId")).intValue();
            priority = ((Long) connection.get("Priority")).intValue();            
            functionId = ((Long) connection.get("FunctionId")).intValue();
            hostName  = ((String) connection.get("HostName"));
            hostPort = ((Long) connection.get("HostPort")).intValue();
            String tmpString = (String) connection.get("InitBytes");
            initSequenceSizeMultiplier = ((Long) connection.get("InitBytesMultiplier")).intValue();
            //the hex representation is double the size in bytes because each element is represented as a char and not a nible
            initSequence = new VdmsTransaction( (tmpString.length() / 2) * initSequenceSizeMultiplier , Hex.decodeHex(tmpString));
            passListId = -1;
            try
            { 
                passListId = ((Long) connection.get("PassListId")).intValue();
            }
            catch(NullPointerException e)
            {
                //do nothing if we do not have a passListId
            }
     
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
    
    public VdmsTransaction GetInitSequence()
    {
        return initSequence;
    }

    public int GetPassListId()
    {
        return passListId;
    }
    
    public abstract void Close();

    public abstract void WriteInitMessage();
    
    public abstract void Write(VdmsTransaction outMessage);
    
    public abstract void WriteExtended(VdmsTransaction outMessage);

    public abstract VdmsTransaction Read();

    public abstract VdmsTransaction ReadExtended();
    
}