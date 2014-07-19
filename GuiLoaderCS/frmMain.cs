using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace GuiLoaderCS
{
    public partial class frmMain : Form
    {
        // http://pinvoke.net/default.aspx/kernel32.LoadLibrary
        [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern IntPtr LoadLibrary(string lpFileName);

        // http://www.pinvoke.net/default.aspx/kernel32/GetProcAddress.html
        [DllImport("kernel32", CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)]
        static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private delegate void InstallHook(IntPtr hwnd, string targetName);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void ReleaseHook();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private delegate Int32 InjectDll(IntPtr hwnd, string targetName, Int32 procId);

        private InstallHook InstallHookFunc = null;
        private ReleaseHook ReleaseHookFunc = null;
        private InjectDll InjectDllFunc = null;

        public frmMain()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            OpenFileDialog fd = new OpenFileDialog();

            if (string.IsNullOrWhiteSpace(tbDllPath.Text))
            {
                fd.InitialDirectory = Application.StartupPath;
            }
            else
            {
                fd.FileName = tbDllPath.Text;
            }
            fd.Filter = "DLL files (*.dll)|*.dll|All files (*.*)|*.*";
            fd.FilterIndex = 2;
            fd.RestoreDirectory = true;

            if (fd.ShowDialog() == DialogResult.OK)
            {
                tbDllPath.Text = fd.FileName;
                Properties.Settings.Default.DLL_PATH = tbDllPath.Text;
            }
        }

        private void frmMain_FormClosing(object sender, FormClosingEventArgs e)
        {
            Properties.Settings.Default.DLL_PATH = tbDllPath.Text;
            Properties.Settings.Default.TARGET_NAME = tbProcessName.Text;
            int idx = 0;
            foreach (RadioButton control in gbInjectionMethod.Controls)
            {
                if (control.Checked)
                    break;
                idx++;
            }
            Properties.Settings.Default.INJECTION_METHOD = idx;
            Properties.Settings.Default.Save();

            if (ReleaseHookFunc != null) ReleaseHookFunc();       
        }

        private void frmMain_Load(object sender, EventArgs e)
        {
            tbDllPath.Text = Properties.Settings.Default.DLL_PATH;
            tbProcessName.Text = Properties.Settings.Default.TARGET_NAME;
            RadioButton activeRB = (RadioButton) gbInjectionMethod.Controls[Properties.Settings.Default.INJECTION_METHOD];
            activeRB.Checked = true;
        }

        private void button2_Click(object sender, EventArgs e)
        {
            IntPtr hMod = LoadLibrary(tbDllPath.Text);
            if (rbManualMap.Checked)
            {
                if (hMod != IntPtr.Zero)
                {
                    IntPtr InjectDllPtr = GetProcAddress(hMod, "InjectDll");
                    InjectDllFunc = (InjectDll)Marshal.GetDelegateForFunctionPointer(InjectDllPtr, typeof(InjectDll));
                }

                if (InjectDllFunc != null)
                {
                    foreach( Process proc in Process.GetProcessesByName(tbProcessName.Text) )
                    {
                        Int32 rval = InjectDllFunc(this.Handle, tbDllPath.Text, proc.Id);
                        Log(string.Format("Injected {0} into process {1}.  rval = {2}\n", tbDllPath.Text, proc.Id, rval));
                    }
                    button2.Enabled = false;
                }
            }
            else if (rbWindowHook.Checked)
            {
                if (hMod != IntPtr.Zero)
                {
                    IntPtr InstallHookPtr = GetProcAddress(hMod, "InstallHook");
                    IntPtr ReleaseHookPtr = GetProcAddress(hMod, "ReleaseHook");

                    InstallHookFunc = (InstallHook)Marshal.GetDelegateForFunctionPointer(InstallHookPtr, typeof(InstallHook));
                    ReleaseHookFunc = (ReleaseHook)Marshal.GetDelegateForFunctionPointer(ReleaseHookPtr, typeof(ReleaseHook));
                }

                if (InstallHookFunc != null)
                {
                    InstallHookFunc(this.Handle, tbProcessName.Text);
                    button2.Enabled = false;
                    Log("Created global window hook.  Launch the target process now...\n");
                }
            }
        }

        private delegate void UpdateLog(string msg);
        private void Log(string msg)
        {
            if (this.tbLog.InvokeRequired)
            {
                this.Invoke(new UpdateLog(Log), msg);
            }
            else
            {
                tbLog.AppendText(msg);
            }
        }

        protected override void WndProc(ref Message m)
        {
            if (m.Msg == 0xBEEF)
            {
                Log(string.Format("Received message from hook DLL! PID={0}\n",m.WParam));
            }
            else
            {
                base.WndProc(ref m);
            }
        }
    }
}
