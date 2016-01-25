// Xamarin/C# app voor de besturing van een Arduino (Uno with Ethernet Shield) m.b.v. een socket-interface.
// Arduino server: DomoticaServer.ino
// De besturing heeft betrekking op het aan- en uitschakelen van een Arduino pin, 
// waar een led aan kan hangen of, t.b.v. het Domotica project, een RF-zender waarmee een 
// klik-aan-klik-uit apparaat bestuurd kan worden.
//
// De app heeft twee modes die betrekking hebben op de afhandeling van de socket-communicatie: "simple-mode" en "threaded-mode" 
// Wanneer het statement    //connector = new Connector(this);    wordt uitgecommentarieerd draait de app in "simple-mode",
// Het opvragen van gegevens van de Arduino (server) wordt dan met een Timer gerealisseerd. (De extra classes Connector.cs, 
// Receiver.cs en Sender.cs worden dan niet gebruikt.) 
// Als er een connector wordt aangemaakt draait de app in "threaded mode". De socket-communicatie wordt dan afgehandeld
// via een Sender- en een Receiver klasse, die worden aangemaakt in de Connector klasse. Deze threaded mode 
// biedt een generiekere en ook robuustere manier van communicatie, maar is ook moeilijker te begrijpen. 
// Aanbeveling: start in ieder geval met de simple-mode
//
// Werking: De communicatie met de (Arduino) server is gebaseerd op een socket-interface. Het IP- en Port-nummer
// is instelbaar. Na verbinding kunnen, middels een eenvoudig commando-protocol, opdrachten gegeven worden aan 
// de server (bijv. pin aan/uit). Indien de server om een response wordt gevraagd (bijv. led-status of een
// sensorwaarde), wordt deze in een 4-bytes ASCII-buffer ontvangen, en op het scherm geplaatst. Alle commando's naar 
// de server zijn gecodeerd met 1 char. Bestudeer het protocol in samenhang met de code van de Arduino server.
// Het default IP- en Port-nummer (zoals dat in het GUI verschijnt) kan aangepast worden in de file "Strings.xml". De
// ingestelde waarde is gebaseerd op je eigen netwerkomgeving, hier, en in de Arduino-code, is dat een router, die via DHCP
// in het segment 192.168.1.x vanaf systeemnummer 100 IP-adressen uitgeeft.
// 
// Resource files:
//   Main.axml (voor het grafisch design, in de map Resources->layout)
//   Strings.xml (voor alle statische strings in het interface, in de map Resources->values)
// 
// De software is verder gedocumenteerd in de code. Tijdens de colleges wordt er nadere uitleg over gegeven.
// 
// Versie 1.0, 12/12/2015
// D. Bruin
// S. Oosterhaven
// W. Dalof (voor de basis van het Threaded interface)
//
using System;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Timers;
using Android.App;
using Android.Content;
using Android.Runtime;
using Android.Views;
using Android.Widget;
using Android.OS;
using Android.Content.PM;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using Android.Graphics;
using System.Threading.Tasks;

namespace Domotica
{
    [Activity(Label = "@string/application_name", MainLauncher = true, Icon = "@drawable/icon", ScreenOrientation = Android.Content.PM.ScreenOrientation.Portrait)]

    public class MainActivity : Activity
    {
        // Variables (components/controls)
        // Controls on GUI
        LinearLayout linearLayout1;
        Button buttonConnect;
        Button buttonStartTimer;
        TextView textViewServerConnect, textViewTimerStateValue, textViewTimerState, textViewServer,
                 textViewIPAddress, textViewSensor, textViewTempSensor, textView1, textView2, textView3,
                 textView4, textView5, textView6, textViewRadioBtn, textViewTime;
        public TextView textViewSensorValue, textViewTempValue;
        EditText editTextIPAddress, editTextMinutes, editTextSeconds;
        Switch switch1, switch2, switch3, switch4, switchLightSensor, switchTempSensor;
        RadioButton radioButton1, radioButton2, radioButton3;
        Button buttonAllOn, buttonAllOff;
        ImageButton buttonStartMusic, buttonStartMusic2, buttonStopMusic, buttonStopMusic2;

        Timer timerClock, timerSockets;             // Timers   
        Socket socket = null;                       // Socket   
        Connector connector = null;                 // Connector (simple-mode or threaded-mode)
        List<Tuple<string, TextView>> commandList = new List<Tuple<string, TextView>>();  // List for commands and response places on UI
        int listIndex = 0;
        int chosenRadioButton = 1;
        int seconds = 0;
        int minutes = 0;
        int delay = 0;
        int colorIndex = 0;
        bool autoConnected = false;
        Color textColor = Color.White;
        Color bgColor = Color.Rgb(9, 153, 204);

        protected override void OnCreate(Bundle bundle)
        {
            base.OnCreate(bundle);

            // Set our view from the "main" layout resource (strings are loaded from Recources -> values -> Strings.xml)
            SetContentView(Resource.Layout.Main);

            // find and set the controls, so it can be used in the code
            linearLayout1 = FindViewById<LinearLayout>(Resource.Id.linearLayout1);
            buttonConnect = FindViewById<Button>(Resource.Id.buttonConnect);
            buttonStartTimer = FindViewById<Button>(Resource.Id.buttonStartTimer);
            switch1 = FindViewById<Switch>(Resource.Id.switch1);
            switch2 = FindViewById<Switch>(Resource.Id.switch2);
            switch3 = FindViewById<Switch>(Resource.Id.switch3);
            switch4 = FindViewById<Switch>(Resource.Id.switch4);
            textViewTimerState = FindViewById<TextView>(Resource.Id.textViewTimerState);
            textViewTimerStateValue = FindViewById<TextView>(Resource.Id.textViewTimerStateValue);
            textViewServer = FindViewById<TextView>(Resource.Id.textViewServer);
            textViewServerConnect = FindViewById<TextView>(Resource.Id.textViewServerConnect);
            textViewSensor = FindViewById <TextView>(Resource.Id.textViewSensor);
            switchLightSensor = FindViewById<Switch>(Resource.Id.switchLightSensor);
            textViewSensorValue = FindViewById<TextView>(Resource.Id.textViewSensorValue);
            textViewTempSensor = FindViewById<TextView>(Resource.Id.textViewTempSensor);
            switchTempSensor = FindViewById<Switch>(Resource.Id.switchTempSensor);
            textViewTempValue = FindViewById<TextView>(Resource.Id.textViewTempValue);
            textViewIPAddress = FindViewById<TextView>(Resource.Id.textViewIPAddress);
            editTextIPAddress = FindViewById<EditText>(Resource.Id.editTextIPAddress);
            editTextMinutes = FindViewById<EditText>(Resource.Id.editTextMinutes);
            editTextSeconds = FindViewById<EditText>(Resource.Id.editTextSeconds);
            textView1 = FindViewById<TextView>(Resource.Id.textView1);
            textView2 = FindViewById<TextView>(Resource.Id.textView2);
            textView3 = FindViewById<TextView>(Resource.Id.textView3);
            textView4 = FindViewById<TextView>(Resource.Id.textView4);
            textView5 = FindViewById<TextView>(Resource.Id.textView5);
            textView6 = FindViewById<TextView>(Resource.Id.textView6);
            textViewRadioBtn = FindViewById<TextView>(Resource.Id.textViewRadioBtn);
            radioButton1 = FindViewById<RadioButton>(Resource.Id.radioButton1);
            radioButton2 = FindViewById<RadioButton>(Resource.Id.radioButton2);
            radioButton3 = FindViewById<RadioButton>(Resource.Id.radioButton3);
            buttonAllOn = FindViewById<Button>(Resource.Id.buttonAllOn);
            buttonAllOff = FindViewById<Button>(Resource.Id.buttonAllOff);
            textViewTime = FindViewById<TextView>(Resource.Id.textViewTime);
            buttonStartMusic = FindViewById<ImageButton>(Resource.Id.buttonStartMusic);
            buttonStopMusic = FindViewById<ImageButton>(Resource.Id.buttonStopMusic);
            buttonStartMusic2 = FindViewById<ImageButton>(Resource.Id.buttonStartMusic2);
            buttonStopMusic2 = FindViewById<ImageButton>(Resource.Id.buttonStopMusic2);
            UpdateConnectionState(4, "Disconnected");

            // Init commandlist, scheduled by socket timer
            commandList.Add(new Tuple<string, TextView>("a", textViewSensorValue)); // ask for temperature sensor value every few seconds
            commandList.Add(new Tuple<string, TextView>("b", textViewTempValue)); // same for light sensor

            // activation of connector -> threaded sockets otherwise -> simple sockets 
            // connector = new Connector(this);

            this.Title = (connector == null) ? this.Title + " (simple sockets)" : this.Title + " (thread sockets)";

            // timer object, running clock
            timerClock = new System.Timers.Timer() { Interval = 1000, Enabled = true }; // Interval >= 1000
            timerClock.Elapsed += (obj, args) =>
            {
                RunOnUiThread(() => { textViewTimerStateValue.Text = DateTime.Now.ToString("hh:mm:ss"); }); //view current time
            };

            // timer object, check Arduino state
            // Only one command can be serviced in an timer tick, schedule from list
            timerSockets = new System.Timers.Timer() { Interval = 1000, Enabled = false }; // Interval >= 750
            timerSockets.Elapsed += (obj, args) =>
            {
                //RunOnUiThread(() =>
                //{
                if (socket != null) // only if socket exists
                {
                    // Send a command to the Arduino server on every tick (loop though list)
                    UpdateGUI(executeCommand(commandList[listIndex].Item1), commandList[listIndex].Item2);  //e.g. UpdateGUI(executeCommand("s"), textViewChangePinStateValue);
                    if (++listIndex >= commandList.Count) listIndex = 0;
                }
                else timerSockets.Enabled = false;  // If socket broken -> disable timer
                //});
            };

            if (!autoConnected) /// 
            {
                for (int i = 100; i < 102; i++)
                {
                    string ip = "192.168.1." + Convert.ToString(i);
                    //Validate the user input (IP address and port)
                    if (CheckValidIpAddress(ip) && CheckValidPort("3300"))
                    {

                        ConnectSocket(ip, "3300"); //simple sockets
                        autoConnected = true;
                        editTextIPAddress.Text = ip;
                    }
                    else UpdateConnectionState(3, "Please check IP");
                }
            }

            //Add the "Connect" button handler.
            if (buttonConnect != null)  // if button exists
            {
                buttonConnect.Click += (sender, e) =>
                {
                    //Validate the user input (IP address and port)
                    if (CheckValidIpAddress(editTextIPAddress.Text) && CheckValidPort("3300"))
                    {
                            ConnectSocket(editTextIPAddress.Text, "3300");
                    }
                    else UpdateConnectionState(3, "Please check IP");
                };
            }
            switchLightSensor.CheckedChange += delegate (object sender, CompoundButton.CheckedChangeEventArgs e)
            {
                socket.Send(Encoding.ASCII.GetBytes("l")); // Send toggle-command to the Arduino to toggle lightsensor
            };

            switchTempSensor.CheckedChange += delegate (object sender, CompoundButton.CheckedChangeEventArgs e)
            {
                socket.Send(Encoding.ASCII.GetBytes("t")); // Send toggle-command to the Arduino to toggle temp sensor
            };

            switch1.CheckedChange += delegate (object sender, CompoundButton.CheckedChangeEventArgs e)
            {
                socket.Send(Encoding.ASCII.GetBytes("1")); // Send toggle-command to the Arduino to toggle RF-switch 1
            };

            switch2.CheckedChange += delegate (object sender, CompoundButton.CheckedChangeEventArgs e)
            {
                socket.Send(Encoding.ASCII.GetBytes("2")); // Send toggle-command to the Arduino to toggle RF-switch 2
            };

            switch3.CheckedChange += delegate (object sender, CompoundButton.CheckedChangeEventArgs e)
            {
                socket.Send(Encoding.ASCII.GetBytes("3")); // Send toggle-command to the Arduino to toggle RF-switch 3
            };
            buttonAllOn.Click += (sender, e) =>
            {
                socket.Send(Encoding.ASCII.GetBytes("4"));
            };
            buttonAllOff.Click += (sender, e) =>
            {
                socket.Send(Encoding.ASCII.GetBytes("5"));
            };
            switch4.CheckedChange += delegate (object sender, CompoundButton.CheckedChangeEventArgs e)
            {
                socket.Send(Encoding.ASCII.GetBytes("g")); // Send toggle-command to the Arduino to toggle GroupMode
            };

                // create handlers for the sleecting of every radiobutton
                radioButton1.Click += RadioButtonClick;
                radioButton2.Click += RadioButtonClick;
                radioButton3.Click += RadioButtonClick;

                // add the eventHandler for the Start Timer button
                buttonStartTimer.Click += (sender, e) =>
                {
                    minutes = Convert.ToInt32(editTextMinutes.Text);
                    seconds = Convert.ToInt32(editTextSeconds.Text);
                    timer(minutes, seconds); 
                    // execute the function timer with the values the user typed in the editTexts as parameters               
                };

            buttonStartMusic.Click += (sender,e) =>
            {
                socket.Send(Encoding.ASCII.GetBytes("m"));  // send command to the arduino to start playing a song
            };

            buttonStopMusic.Click += (sender, e) =>
            {
                socket.Send(Encoding.ASCII.GetBytes("n")); // send command to the arduino to stop playing the song
            };

            buttonStartMusic2.Click += (sender, e) =>
            {
                socket.Send(Encoding.ASCII.GetBytes("o"));  // send command to the arduino to start playing a song
            };

            buttonStopMusic2.Click += (sender, e) =>
            {
                socket.Send(Encoding.ASCII.GetBytes("p")); // send command to the arduino to stop playing the song
            };

        }

        //Send command to server and wait for response (blocking)
        //Method should only be called when socket existst
        public string executeCommand(string cmd)
        {
            byte[] buffer = new byte[4]; // response is always 4 bytes
            int bytesRead = 0;
            string result = "---";

            if (socket != null)
            {
                //Send command to server
                socket.Send(Encoding.ASCII.GetBytes(cmd));

                try //Get response from server
                {
                    //Store received bytes (always 4 bytes, ends with \n)
                    bytesRead = socket.Receive(buffer);  // If no data is available for reading, the Receive method will block until data is available,
                    //Read available bytes.              // socket.Available gets the amount of data that has been received from the network and is available to be read
                    while (socket.Available > 0) bytesRead = socket.Receive(buffer);
                    if (bytesRead == 4)
                        result = Encoding.ASCII.GetString(buffer, 0, bytesRead - 1); // skip \n
                    else result = "err";
                }
                catch (Exception exception)
                {
                    // if the bytes could not be received or not be read
                    result = exception.ToString();
                    if (socket != null)
                    {
                        socket.Close();
                        socket = null;
                    }
                    UpdateConnectionState(3, result);
                }
            }
            return result;
        }

        private void RadioButtonClick(object sender, EventArgs e)
        {
            RadioButton rb = (RadioButton)sender; // select the radiobutton that was clicked, deselect the others
            chosenRadioButton = Convert.ToInt32(rb.Text); // update the value of which radiobutton is selected
        }

        //Update connection state label (GUI).
        public void UpdateConnectionState(int state, string text)
        {
            // connectButton
            string butConText = "Connect";  // default text
            bool butConEnabled = true;      // default state
            Color color = Color.Red;        // default color
            // pinButton

            //Set "Connect" button label according to connection state.
            if (state == 1)
            {
                butConText = "Please wait";
                color = Color.Orange;
                butConEnabled = false;
            }
            else
            if (state == 2)
            {
                butConText = "Disconnect";
                color = Color.Green;
            }
            //Edit the control's properties on the UI thread
            RunOnUiThread(() =>
            {
                textViewServerConnect.Text = text;
                if (butConText != null)  // text existst
                {
                    buttonConnect.Text = butConText;
                    textViewServerConnect.SetTextColor(color);
                    buttonConnect.Enabled = butConEnabled;
                }
            });
        }

        //Update GUI based on Arduino response
        public void UpdateGUI(string result, TextView textview)
        {
            RunOnUiThread(() =>
            {
                if (result == "OFF") textview.SetTextColor(Color.Red);
                else if (result == " ON") textview.SetTextColor(Color.Green);
                textview.Text = result;
            });
        }

        // Connect to socket ip/prt (simple sockets)
        public void ConnectSocket(string ip, string prt)
        {
            RunOnUiThread(() =>
            {
                if (socket == null)                                       // create new socket
                {
                    UpdateConnectionState(1, "Connecting...");
                    try  // to connect to the server (Arduino).
                    {
                        socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
                        socket.Connect(new IPEndPoint(IPAddress.Parse(ip), Convert.ToInt32(prt)));
                        if (socket.Connected)
                        {
                            UpdateConnectionState(2, "Connected");
                            timerSockets.Enabled = true;                //Activate timer for communication with Arduino     
                        }
                    }
                    catch (Exception exception)
                    {
                        timerSockets.Enabled = false;
                        if (socket != null)
                        {
                            socket.Close();
                            socket = null;
                        }
                        UpdateConnectionState(4, exception.Message);
                    }
                }
                else // disconnect socket
                {
                    socket.Close(); socket = null;
                    timerSockets.Enabled = false;
                    UpdateConnectionState(4, "Disconnected");
                }
            });
        }

        //Close the connection (stop the threads) if the application stops.
        protected override void OnStop()
        {
            base.OnStop();

            if (connector != null)
            {
                if (connector.CheckStarted())
                {
                    connector.StopConnector();
                }
            }
        }

        //Close the connection (stop the threads) if the application is destroyed.
        protected override void OnDestroy()
        {
            base.OnDestroy();

            if (connector != null)
            {
                if (connector.CheckStarted())
                {
                    connector.StopConnector();
                }
            }
        }

        //Prepare the Screen's standard options menu to be displayed.
        public override bool OnPrepareOptionsMenu(IMenu menu)
        {
            //Prevent menu items from being duplicated.
            menu.Clear();

            MenuInflater.Inflate(Resource.Menu.menu, menu);
            return base.OnPrepareOptionsMenu(menu);
        }

        //Executes an action when a menu button is pressed.
        public override bool OnOptionsItemSelected(IMenuItem item)
        {
            switch (item.ItemId)
            {
                case Resource.Id.theme:
                    colorIndex ++;      //Goes through all the available background colors
                    if (colorIndex == 4) { colorIndex = 0; }       
                    switch (colorIndex)
                    {
                        case 0:
                            changeColor("blue"); //sends the chosen background color to the function changecolor.
                            break;
                        case 1:
                            changeColor("yellow");
                            break;
                        case 2:
                            changeColor("green");
                            break;
                        case 3:
                            changeColor("orange");
                            break;                 
                    }
                    return true;
                case Resource.Id.exit:
                    //Force quit the application.
                    System.Environment.Exit(0);
                    return true;
                case Resource.Id.abort:

                    //Stop threads forcibly (for debugging only).
                    if (connector != null)
                    {
                        if (connector.CheckStarted()) connector.Abort();
                    }
                    return true;
            }
            return base.OnOptionsItemSelected(item);
        }

        //Check if the entered IP address is valid.
        private bool CheckValidIpAddress(string ip)
        {
            if (ip != "")
            {
                //Check user input against regex (check if IP address is not empty).
                Regex regex = new Regex("\\b((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)(\\.|$)){4}\\b");
                Match match = regex.Match(ip);
                return match.Success;
            }
            else return false;
        }

        public async void timer(int m, int s)
        {
            delay = 60000 * m + 1000 * s;  // set a delay for the time the user typed
            await WaitMethod();

            // toggle the switch that belongs to the chosen radiobutton.
            if (chosenRadioButton == 1) switch1.Toggle();
            else if (chosenRadioButton == 2) switch2.Toggle();
            else if (chosenRadioButton == 3) switch3.Toggle();
        }

        async System.Threading.Tasks.Task WaitMethod()
        {
            await System.Threading.Tasks.Task.Delay(delay); // wait for the delay
        }

        public void changeColor(string col)
        {
            List<TextView> textViews = new List<TextView>
            {    textViewTimerStateValue, textViewTimerState, textViewServer,
                 textViewIPAddress, textViewSensor, textViewTempSensor, textView1, textView2, textView3,
                 textView4, textView5, textView6, textViewRadioBtn, textViewTime, textViewSensorValue, textViewTempValue
            };
            List<EditText> editTexts = new List<EditText>{ editTextIPAddress, editTextMinutes, editTextSeconds };
            List<RadioButton> radioButtons = new List<RadioButton> { radioButton1, radioButton2, radioButton3 };

            if (col == "blue") { textColor = Color.White; bgColor = Color.Rgb(9, 153, 204); }
            else if (col == "yellow") { textColor = Color.Black; bgColor = Color.Yellow; }
            else if (col == "green") { textColor = Color.White; bgColor = Color.DarkGreen; }
            else if (col == "orange") { textColor = Color.Black; bgColor = Color.DarkOrange; }

            linearLayout1.SetBackgroundColor(bgColor); // changes backgroundcolor to the chosen backgroundcolor
            foreach (TextView t in textViews) { t.SetTextColor(textColor); } //changes color of all text to chosen color
            foreach (RadioButton r in radioButtons) { r.SetTextColor(textColor); }
            foreach (EditText e in editTexts) { e.SetBackgroundColor(textColor); e.SetTextColor(bgColor); }

        }

        //Check if the entered port is valid.
        private bool CheckValidPort(string port)
        {
            //Check if a value is entered.
            if (port != "")
            {
                Regex regex = new Regex("[0-9]+");
                Match match = regex.Match(port);

                if (match.Success)
                {
                    int portAsInteger = Int32.Parse(port);
                    //Check if port is in range.
                    return ((portAsInteger >= 0) && (portAsInteger <= 65535));
                }
                else return false;
            }
            else return false;
        }
    }
}

        
