import java.io.IOException;
import java.io.DataInputStream;
import java.io.DataOutputStream;

import java.net.Socket;
import java.util.ArrayList;
import java.util.Arrays;
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
    Socket serverSocket;
    boolean m_bRunThread = true;
    int id;
    int type;
    int messageId;
    Plugin manager;
    BlockingQueue<VdmsTransaction> responseQueue;
    VdmsTransaction initSequence;
    ArrayList<String> passList;
    
    public SubscriberServiceThread()
    { 
        super();
        responseQueue = null;
        initSequence = null;
        passList = null;
        
    } 
    
    public SubscriberServiceThread(Plugin nManager, Socket s, int nThreadId, ArrayList<String> nPassList, VdmsTransaction nInitSequence) 
    {
        responseQueue = new ArrayBlockingQueue<VdmsTransaction>(128);
        manager = nManager;
        serverSocket = s;
        id = nThreadId;
        messageId = 0;
        initSequence = nInitSequence;
        passList = nPassList;
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
                    for(String checkString : passList)
                    {
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
        DataInputStream in = null; 
        DataOutputStream out = null;
        byte[] readSizeArray = new byte[4];
        
        
        int int0;
        int int1;
        int int2;
        int int3;
        int readSize;
        VdmsTransaction returnedMessage;
        Boolean threadInitFlag = false;
        VdmsTransaction newTransaction = null;
        
        try
        { 
            in = new DataInputStream(serverSocket.getInputStream());
            out = new DataOutputStream(serverSocket.getOutputStream());
            
            while(m_bRunThread) 
            {
                if(threadInitFlag == false)
                {
                    //only write the value if there is a valid init sequence
                    if(initSequence != null)
                    {
                        out.write(initSequence.GetSize());
                        out.write(initSequence.GetBuffer());
                    }
                    threadInitFlag = true;
                }
                
                returnedMessage = responseQueue.take();
                //need to check to see if there is a message id - needs to have message id here for pass back up
                out.write(returnedMessage.GetSize());
                out.write(returnedMessage.GetBuffer());                
                ++messageId;
                
                //make sure to pass message Id back up
                in.read(readSizeArray, 0, 4);
                int0 = readSizeArray[0] & 255;
                int1 = readSizeArray[1] & 255;
                int2 = readSizeArray[2] & 255;
                int3 = readSizeArray[3] & 255;
                readSize = int0 + (int1 << 8) + (int2 << 16) + (int3 << 24);
                //now i can read the rest of the data
                System.out.println("readsizearray - " + Arrays.toString(readSizeArray));		
                
                byte[] buffer = new byte[readSize];
                in.read(buffer, 0, readSize);
                System.out.println("buffer - " + Arrays.toString(buffer));
                
                newTransaction = new VdmsTransaction(readSizeArray, buffer);
                //System.out.println("Server Says :" + Integer.toString(tmpInt));
                manager.AddToProducerQueue(newTransaction);
                
                
                
            }
        }
        catch(Exception e) 
        { 
            e.printStackTrace(); 
        } 
        finally 
        { 
            try
            { 
                in.close(); 
                out.close(); 
                serverSocket.close(); 
                System.out.println("...Stopped"); 
            }
            catch(IOException ioe)
            { 
                ioe.printStackTrace(); 
            } 
        } 
    } 
}
