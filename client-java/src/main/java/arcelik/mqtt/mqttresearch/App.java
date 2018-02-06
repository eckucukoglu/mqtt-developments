package arcelik.mqtt.mqttresearch;

public class App {
    
    public static void main( String[] args ) {
        
        MqttConnectionCountTest connectionCountTest = new MqttConnectionCountTest();
        connectionCountTest.test(Integer.valueOf(args[0]));
        System.exit(0);
        
    }
}


