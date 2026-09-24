Imports System.IO
Imports System.IO.Ports
Imports System.Reflection.Emit
Imports System.Runtime.InteropServices
Imports System.Text.Json
Imports System.Windows.Forms.Design.AxImporter
Imports System.Windows.Forms.VisualStyles.VisualStyleElement
Imports System.Windows.Forms.VisualStyles.VisualStyleElement.Button
Imports System.Xml
Imports NAudio.CoreAudioApi

Public Class gunmu
    Private WithEvents trayIcon As New NotifyIcon()
    Private WithEvents trayMenu As New ContextMenuStrip()

    Dim PriSp As System.IO.Ports.SerialPort
    Private portSelectMenu As ToolStripMenuItem
    Private connectItem As ToolStripMenuItem
    Private conItem As ToolStripMenuItem, autoBoot As ToolStripMenuItem, portItem As ToolStripMenuItem
    Private ReadOnly configPath As String = Path.Combine(Application.StartupPath, "config.json")
    Dim VolumeLastFrame As Single = 0

    Private Sub PortItemClickHandler(sender As Object, e As EventArgs)
        Dim clickedItem As ToolStripMenuItem = CType(sender, ToolStripMenuItem)
        For Each item As ToolStripItem In portSelectMenu.DropDownItems
            If TypeOf item Is ToolStripMenuItem Then
                Dim mi As ToolStripMenuItem = CType(item, ToolStripMenuItem)
                mi.Checked = False
            End If
        Next
        clickedItem.Checked = True
        portSelectMenu.Text = $"串口选择: {clickedItem.Text.Split(":"c)(0)}"
    End Sub

    Private Sub connect(portNumber As String)
        For Each item As ToolStripItem In trayMenu.Items
            If item.Text.Contains("连接") Then
                conItem = item
            End If
        Next
        Dim ConResult As Boolean = False
        Try
            If PriSp IsNot Nothing AndAlso PriSp.IsOpen Then
                PriSp.Close()
            End If
            PriSp = New System.IO.Ports.SerialPort(portNumber, 9600)
            PriSp.DtrEnable = True
            PriSp.RtsEnable = True
            PriSp.ReadTimeout = 500
            PriSp.WriteTimeout = 500
            Dim resp As String = ""
            Dim lockObj As New Object()
            AddHandler PriSp.DataReceived, Sub(s, args)
                                               Try
                                                   Dim spInst = CType(s, System.IO.Ports.SerialPort)
                                                   Dim text As String = spInst.ReadExisting()
                                                   SyncLock lockObj
                                                       resp &= text
                                                   End SyncLock
                                               Catch
                                               End Try
                                           End Sub

            PriSp.Open()
            System.Threading.Thread.Sleep(1500)
            PriSp.WriteLine("CONNECTING_")
            Dim startTime As DateTime = DateTime.Now
            While (DateTime.Now - startTime).TotalMilliseconds < 2000
                SyncLock lockObj
                    If resp.Contains("CON_DONE_") Then
                        ConResult = True
                        Exit While
                    End If
                End SyncLock
                System.Threading.Thread.Sleep(50)
            End While
            If ConResult Then
                conItem.Text = "断开连接"
                portItem.Enabled = False
                Timer1.Enabled = True
            Else
                PriSp.Close()
                conItem.Text = "连接超时，重新连接"
            End If
        Catch ex As Exception
            If PriSp IsNot Nothing AndAlso PriSp.IsOpen Then
                PriSp.Close()
            End If
            conItem.Text = "连接失败，重新连接"
        End Try
    End Sub

    Private Sub gunmu_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        InitTrayIcon()
        For Each item As ToolStripItem In trayMenu.Items
            If item.Text.Contains("开机自启") Then
                autoBoot = item
            End If
        Next
        For Each item As ToolStripItem In trayMenu.Items
            If item.Text.Contains("串口选择") Then
                portItem = item
            End If
        Next
        If Not File.Exists(configPath) Then
            Dim config As New Conf()
            Dim options As New JsonSerializerOptions With
            {
                .WriteIndented = True
            }
            config.startup = False
            Dim jsonText As String = JsonSerializer.Serialize(config, options)
            File.WriteAllText(configPath, jsonText)
        Else
            Try
                Dim jsonText As String = File.ReadAllText(configPath)
                Dim config As Conf = JsonSerializer.Deserialize(Of Conf)(jsonText)
                If config IsNot Nothing Then
                    autoBoot.Checked = config.startup
                End If
            Catch ex As Exception
            End Try
        End If
        If autoBoot.Checked = True Then
            connect(portItem.Text?.ToString.Split(" "c)(1))
        End If
        Me.Visible = False
    End Sub

    Private Sub InitTrayIcon()
        trayIcon.Icon = Me.Icon
        trayIcon.Text = "音量调节器运行中"
        trayIcon.Visible = True
        portSelectMenu = New ToolStripMenuItem("串口选择")
        Dim ports As String() = SerialPort.GetPortNames()
        Dim defaultPort As String = ""
        For Each portName As String In ports
            Dim HSSuccess As Boolean = False
            Using sp As New System.IO.Ports.SerialPort(portName, 9600)
                sp.DtrEnable = True
                sp.RtsEnable = True
                sp.ReadTimeout = 300
                sp.WriteTimeout = 300
                Dim resp As String = ""
                Dim lockObj As New Object()
                AddHandler sp.DataReceived, Sub(s, args)
                                                Try
                                                    Dim serialPortInstance = CType(s, System.IO.Ports.SerialPort)
                                                    Dim text As String = serialPortInstance.ReadExisting()
                                                    SyncLock lockObj
                                                        resp &= text
                                                    End SyncLock
                                                Catch
                                                End Try
                                            End Sub
                Try
                    sp.Open()
                    System.Threading.Thread.Sleep(1500)
                    sp.WriteLine("INEEDU")
                    Dim startTime As DateTime = DateTime.Now
                    While (DateTime.Now - startTime).TotalMilliseconds < 1000
                        SyncLock lockObj
                            If resp.Contains("IMCOMING") Then
                                HSSuccess = True
                                Exit While
                            End If
                        End SyncLock
                        System.Threading.Thread.Sleep(50)
                    End While
                Catch ex As Exception
                    HSSuccess = False
                End Try
            End Using
            Dim PortSelectItem As String = portName
            If HSSuccess Then
                PortSelectItem = portName & ": 发现音量调节器"
                If String.IsNullOrEmpty(defaultPort) Then
                    defaultPort = PortSelectItem
                End If
            End If
            Dim subItem As New ToolStripMenuItem(PortSelectItem)
            AddHandler subItem.Click, AddressOf PortItemClickHandler
            portSelectMenu.DropDownItems.Add(subItem)
        Next
        If Not String.IsNullOrEmpty(defaultPort) Then
            For Each menuItem As ToolStripItem In portSelectMenu.DropDownItems
                If TypeOf menuItem Is ToolStripMenuItem Then
                    Dim portItem As ToolStripMenuItem = CType(menuItem, ToolStripMenuItem)
                    If portItem.Text = defaultPort Then
                        portItem.Checked = True
                        portSelectMenu.Text = $"串口选择: {defaultPort.ToString.Split(":"c)(0)}"
                    Else
                        portItem.Checked = False
                    End If
                End If
            Next
        End If
        connectItem = New ToolStripMenuItem("连接")
        AddHandler connectItem.Click, Sub(s, ev)
                                          If connectItem.Text <> "断开连接" Then
                                              Try
                                                  connect(portItem.Text?.ToString.Split(" "c)(1))
                                              Catch ex As Exception
                                                  MessageBox.Show("未选择串口或串口不可用！", "错误", MessageBoxButtons.OK, MessageBoxIcon.Error)
                                              End Try
                                          Else
                                              Timer1.Enabled = False
                                              connectItem.Text = "连接"
                                              portItem.Enabled = True
                                              PriSp.Close()
                                          End If
                                      End Sub
        Dim exitMenuItem As New ToolStripMenuItem("退出")
        AddHandler exitMenuItem.Click, Sub(s, ev)
                                           Application.Exit()
                                       End Sub
        Dim startAtLogon As New ToolStripMenuItem("开机自启")
        AddHandler startAtLogon.Click, Sub(s, ev)
                                           Dim jsonTextOrigin As String = File.ReadAllText(configPath)
                                           Dim config As Conf = JsonSerializer.Deserialize(Of Conf)(jsonTextOrigin)
                                           Dim taskName As String = "FaderVolumeAdjuster"
                                           Dim exePath As String = Application.ExecutablePath
                                           Dim options As New JsonSerializerOptions With
                                           {
                                               .WriteIndented = True
                                           }
                                           If config Is Nothing Then
                                               config = New Conf()
                                           End If
                                           If startAtLogon.Checked = False Then
                                               startAtLogon.Checked = True
                                               config.startup = startAtLogon.Checked
                                               Dim cmd As String = $"schtasks /create /tn ""{taskName}"" /tr ""{exePath}"" /sc ONLOGON /f"
                                               Dim psi As New ProcessStartInfo("cmd.exe", "/c " & cmd) With {
                                                   .CreateNoWindow = True,
                                                   .UseShellExecute = False
                                               }
                                               Process.Start(psi)
                                           ElseIf startAtLogon.Checked = True Then
                                               startAtLogon.Checked = False
                                               config.startup = startAtLogon.Checked
                                               Dim cmd As String = $"schtasks /delete /tn ""{taskName}"" /f"
                                               Dim psi As New ProcessStartInfo("cmd.exe", "/c " & cmd) With {
                                                   .CreateNoWindow = True,
                                                   .UseShellExecute = False
                                               }
                                               Process.Start(psi)
                                           End If
                                           Dim jsonText As String = JsonSerializer.Serialize(config, options)
                                           File.WriteAllText(configPath, jsonText)
                                       End Sub
        trayMenu.Items.Add(portSelectMenu)
        trayMenu.Items.Add(connectItem)
        trayMenu.Items.Add(startAtLogon)
        trayMenu.Items.Add(exitMenuItem)
        trayIcon.ContextMenuStrip = trayMenu
    End Sub

    Private Sub trayIcon_Click(sender As Object, e As EventArgs) Handles trayIcon.Click
        trayMenu.Show()
    End Sub

    Private Sub trayIcon_DoubleClick(sender As Object, e As EventArgs) Handles trayIcon.DoubleClick
        '无需响应。
    End Sub

    Private Sub Timer1_Tick(sender As Object, e As EventArgs) Handles Timer1.Tick
        Dim activePorts As String() = System.IO.Ports.SerialPort.GetPortNames()
        Dim portsString As String = "," & String.Join(",", activePorts) & ","
        Dim enumerator As New MMDeviceEnumerator()
        Dim device As MMDevice = enumerator.GetDefaultAudioEndpoint(DataFlow.Render, Role.Multimedia)
        Dim Volume As Single = device.AudioEndpointVolume.MasterVolumeLevelScalar
        If portsString.Contains("," & PriSp.PortName & ",") Then '先判定音量调节器没有断开再发包。
            PriSp.DtrEnable = True
            PriSp.RtsEnable = True
            PriSp.ReadTimeout = 500
            PriSp.WriteTimeout = 500
            PriSp.WriteLine($"#{CInt(Volume * 100)}|{CInt(VolumeLastFrame * 100)}|{device.FriendlyName}$")
            Timer1.Enabled = True
        ElseIf Not portsString.Contains("," & PriSp.PortName & ",") Then '先判定音量调节器没有断开再发包。
            PriSp.Close()
            MessageBox.Show("音量调节器断开，请重新连接。", "错误", MessageBoxButtons.OK, MessageBoxIcon.Error)
            Application.Exit()
        End If
        VolumeLastFrame = volume
    End Sub
End Class
