import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ArrayBlockingQueue;

class PublisherServiceThread extends Thread
{ 
    VdmsConnection connection;
    boolean m_bRunThread = true;
    int id;
    int type;
    int messageId;
    Plugin manager;
    BlockingQueue<VdmsTransaction> responseQueue;
    VdmsTransaction initSequence;
    
    public PublisherServiceThread()
    { 
        super();
        responseQueue = null;
        initSequence = null;
    } 
    
    PublisherServiceThread(Plugin nManager, VdmsConnection nConnection, int nThreadId) 
    {
        responseQueue = new ArrayBlockingQueue<VdmsTransaction>(128);
        manager = nManager;
        id = nThreadId;
        messageId = 0;
        connection = nConnection;
    } 
    
    
    public void Publish(VdmsTransaction newMessage)
    {
        responseQueue.add(newMessage);
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
                
                newTransaction = connection.ReadExtended();
                manager.AddToConsumerQueue(newTransaction);
                returnedMessage = responseQueue.take();
                connection.WriteExtended(returnedMessage);
                ++messageId;  
                
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
