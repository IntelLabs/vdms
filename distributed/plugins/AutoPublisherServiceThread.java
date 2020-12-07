import java.io.IOException;
import java.io.DataInputStream;
import java.io.DataOutputStream;

import java.net.Socket;

import java.util.Arrays;
import java.util.Timer;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ArrayBlockingQueue;

import java.nio.charset.StandardCharsets;

import java.util.TimerTask;

import VDMS.protobufs.QueryMessage;
import com.google.protobuf.InvalidProtocolBufferException;

import org.json.simple.JSONObject;
import org.json.simple.JSONArray;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

class AutoPublisherServiceThread extends PublisherServiceThread
{ 
    BlockingQueue<VdmsTransaction> outboundAutoQueue;
    int autoQueryDelay;
    int autoQueryPeriod;
    
    public AutoPublisherServiceThread()
    { 
        super();
        autoQueryDelay = 1000;
        autoQueryPeriod = 2000;
    } 
    
    AutoPublisherServiceThread(Plugin nManager, Socket s, int nThreadId, VdmsTransaction nInitSequence) 
    {
        super(nManager, s, nThreadId, nInitSequence);
        outboundAutoQueue = new ArrayBlockingQueue<VdmsTransaction>(128);
        autoQueryDelay = 1000;
        autoQueryPeriod = 2000;
    } 
    
    
    public void Publish(VdmsTransaction newMessage)
    {
        responseQueue.add(newMessage);
    }


    
    public void run() 
    { 
        DataOutputStream out = null;
        VdmsTransaction returnedMessage;
        Boolean threadInitFlag = false;
        VdmsTransaction newTransaction = null;
        
        try
        { 
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


                        AutoQueryTask timedQuery = new AutoQueryTask();
                        Timer t = new Timer();
                        t.schedule(timedQuery, 1000, 1000);
                    }
                    threadInitFlag = true;
                }
                                
                newTransaction =  outboundAutoQueue.take();
                manager.AddToConsumerQueue(newTransaction);
                returnedMessage = responseQueue.take();
                ++messageId;  
                
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



   class AutoQueryTask extends TimerTask 
   {
      private VdmsTransaction autoQuery;

      public AutoQueryTask()
      {











/*

        Calendar calendar = Calendar.getInstance(TimeZone.getTimeZone("UTC"));
        calendar.clear();
        calendar.set(2011, Calendar.OCTOBER, 1);
        long secondsSinceEpoch = calendar.getTimeInMillis() / 1000L;








        The methods Calendar.getTimeInMillis() and Date.getTime() both return milliseconds since 1.1.1970.

        For current time, you can use:
        
        long seconds = System.currentTimeMillis() / 1000l;
        
  */      












        QueryMessage.queryMessage autoMessage = QueryMessage.queryMessage.newBuilder().setJson("[{\'FindEntity\': {\'class\': \'drone\', \'results\': {\'list\': [\'pos_x\', \'pos_y\']}}}]").build();
        int messageSizeInt = autoMessage.getSerializedSize();
        byte messageSize[] = {(byte) (messageSizeInt & 0x000000ff), (byte) ((messageSizeInt & 0x0000ff00) >>> 8), (byte) ((messageSizeInt & 0x00ff0000) >>> 16),  (byte)((messageSizeInt & 0x00ff0000) >>> 24)};
        autoQuery = new VdmsTransaction(messageSize, autoMessage.toByteArray());
      }
      public AutoQueryTask(VdmsTransaction nAutoQuery) 
      {
          autoQuery = nAutoQuery;
      }
      public void run() 
      {
         System.out.println("hello world");
         //outboundAutoQueue.add(autoQuery);
      }
   }

}
