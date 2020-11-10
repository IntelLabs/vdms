import java.util.ArrayList;
import java.util.List;

class VdmsTransaction
{
    byte size[];
    byte buffer[];
    int messageId;
    int threadId;

    public VdmsTransaction(byte[] nSize, byte nBuffer[], int nThreadId)
    {
        size = nSize;
        buffer = nBuffer;
        threadId = nThreadId;
        messageId = -1;
    }

    public byte[] GetSize()
    {
    	return size;
    }

    public byte[] GetBuffer()
    {
	    return buffer;
    }

    public int GetMessageId()
    {
        return messageId;
    }

    public void SetMessageId(int nMessageId)
    {
        messageId = nMessageId;
    }
    public int GetThreadId()
    {
        return threadId;
    }

    public void SetThreadId(int nThreadId)
    {
        threadId = nThreadId;
    }

}
