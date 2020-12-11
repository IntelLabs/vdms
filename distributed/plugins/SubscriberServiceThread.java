import java.util.ArrayList;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ArrayBlockingQueue;

import VDMS.protobufs.QueryMessage;
import com.google.protobuf.InvalidProtocolBufferException;

import org.json.simple.JSONObject;
import org.json.simple.JSONArray;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

class SubscriberServiceThread extends Thread
{
    boolean m_bRunThread = true;
    int id;
    int type;
    int messageId;
    Plugin manager;
    BlockingQueue<VdmsTransaction> responseQueue;
    VdmsTransaction initSequence;
    PassList passList;
    VdmsConnection connection;
    
    public SubscriberServiceThread()
    { 
        super();
        responseQueue = null;
        initSequence = null;
        passList = null;
        connection = null;
        
    } 
    
    public SubscriberServiceThread(Plugin nManager, VdmsConnection nConnection, int nThreadId) 
    {
        responseQueue = new ArrayBlockingQueue<VdmsTransaction>(128);
        manager = nManager;
        connection = nConnection;
        id = nThreadId;
        messageId = 0;
        passList = null;
    } 
    
    public void SetPassList(PassList nPassList)
    {
        passList = nPassList;
    }

    public PassList GetPassList()
    {
        return passList;
    }

    public void Publish(VdmsTransaction newMessage)
    {
        boolean passMessage = false;
        //if there is no passList all values should be forwarded
        if(passList == null)
        {
            responseQueue.add(newMessage);
        }
        else
        {
            try
            {
                QueryMessage.queryMessage newTmpMessage = QueryMessage.queryMessage.parseFrom(newMessage.GetBuffer());
                JSONParser jsonString = new JSONParser();
                JSONArray jsonArray = (JSONArray) jsonString.parse(newTmpMessage.getJson());
                JSONObject jsonObject = (JSONObject) jsonArray.get(0);
                
                //Iterate through the keys in this message and check against the keys that should be published to this node
                for (Object key : jsonObject.keySet()) 
                {
                    for(int i = 0; i < passList.GetSize(); i++)
                    {
                        String checkString = passList.GetValue(i);
                        if(checkString.equals(key))
                        {
                            passMessage = true;
                        }
                    }
                    System.out.println(key.toString());
                }
                
                if(passMessage)
                {
                    responseQueue.add(newMessage);
                }
                
            }
            catch(InvalidProtocolBufferException e)
            {
                System.exit(-1);
            }
            catch(ParseException e)
            {
                System.exit(-1);
            }
        }
        
    }
    
    public void run() 
    { 
        VdmsTransaction returnedMessage;
        Boolean threadInitFlag = false;
        VdmsTransaction newTransaction = null;
        
        try
        { 
            while(m_bRunThread) 
            {
                if(threadInitFlag == false)
                {
                    //only write the value if there is a valid init sequence
                    if(connection.GetInitSequence() != null)
                    {
                        connection.WriteInitMessage();
                    }
                    threadInitFlag = true;
                }
                
                returnedMessage = responseQueue.take();
                //need to check to see if there is a message id - needs to have message id here for pass back up
                manager.AddOutgoingMessageRegistry(returnedMessage.GetId(), id);
                connection.Write(returnedMessage);
                ++messageId;
                
                newTransaction = connection.Read();
                newTransaction.SetId(returnedMessage.GetId());
                newTransaction.SetTimestamp(System.currentTimeMillis() - returnedMessage.GetTimestamp());
                manager.AddToProducerQueue(newTransaction);
            }
        }
        catch(Exception e) 
        { 
            e.printStackTrace(); 
        } 
        finally 
        { 
            connection.Close();
        } 
    } 
}
