import wx
import sys
import time
import socket
import threading
import traceback
import Queue
import serial
import struct
import StringIO

doc_desc = 'Camera Test'
doc_number = ''
doc_rev = 'Rev 4'

exception = ''

class MainFrame(wx.Frame):
	def __init__(self, parent, title):
		super(MainFrame, self).__init__(parent=parent, title=title, size=(800,600),
										style=(wx.DEFAULT_FRAME_STYLE))# & ~(wx.RESIZE_BORDER | wx.RESIZE_BOX | wx.MAXIMIZE_BOX)))
		
		# The panel on which everything will be placed
		self.title_panel = wx.Panel(self)
		self.body_panel = wx.Panel(self)
		
		self.camera_image = wx.Image('capture.bmp', wx.BITMAP_TYPE_ANY).ConvertToBitmap()
		# Add everything
		sizer = wx.BoxSizer(wx.VERTICAL)
		sizer.Add(self.CreateTitle(self.title_panel), flag=wx.GROW)
		sizer.Add(self.CreateBody(self.body_panel))#, flag=wx.GROW)
		self.SetSizerAndFit(sizer)
		self.SetMinSize(self.GetSize())
		
		self.Bind(wx.EVT_CLOSE, self.OnCloseWindow, self)
		
		self.quit = False
		self.q = Queue.Queue(25)
		self.ready = True
		self.tx_string = ''
		
		self.serial_thread = threading.Thread(target=self.Serial_Process)
		self.serial_thread.start()
		
		
		#self.TriggerEvent(wx.wxEVT_COMMAND_RADIOBUTTON_SELECTED, self.mfg_all)
		
		
		# Redirect stdout statements to text control
		#redir=RedirectText(log)
		#sys.stdout=redir
		
	def OnCloseWindow(self, event):
		self.quit = True
		self.Destroy()
		busy = wx.BusyInfo('Please wait, cleaning up ...')
		wx.Yield()
		while self.serial_thread.isAlive():
			time.sleep(0.01)
			wx.Yield()
		
	def CreateTitle(self, panel):
		number = wx.StaticText(panel, label=doc_number)
		rev = wx.StaticText(panel, label=doc_rev)
		desc = wx.StaticText(panel, label=doc_desc)
		font = panel.GetFont()
		#font = wx.Font()
		font.SetWeight(wx.FONTWEIGHT_BOLD)
		font.SetPointSize(font.GetPointSize()+2)
		number.SetFont(font)
		rev.SetFont(font)
		font.SetPointSize(font.GetPointSize()+2)
		desc.SetFont(font)
		
		doc_sizer = wx.BoxSizer(wx.VERTICAL)
		doc_sizer.Add(number, flag=wx.ALIGN_CENTER|wx.RIGHT|wx.LEFT, border=5)
		doc_sizer.Add(rev, flag=wx.ALIGN_CENTER)
		
		title_sizer = wx.BoxSizer(wx.HORIZONTAL)
		title_sizer.Add(doc_sizer, flag=wx.GROW)
		title_sizer.AddStretchSpacer(1)
		title_sizer.Add(desc, flag=wx.ALIGN_CENTER)
		title_sizer.AddStretchSpacer(1)
		
		panel.SetSizerAndFit(title_sizer)
		
		return panel
		
	def CreateBody(self, panel):
		self.mapping = {}
		row_sizer = wx.BoxSizer(wx.VERTICAL)
		image_panel = wx.Panel(panel)
		row_sizer.Add(self.Create_ImagePanel(image_panel), flag=wx.GROW)
		pnl = wx.Panel(panel)
		row_sizer.Add(self.CreateRegPanel(pnl), flag=wx.GROW|wx.RIGHT|wx.LEFT, border=10)
		
		panel.SetSizerAndFit(row_sizer)
		
		return panel
		
	def Create_ImagePanel(self, panel):
		panel.SetMinSize((400, 300))
		panel.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
		
		self.main = panel
		panel.Bind(wx.EVT_PAINT, self.OnPaint)
		panel.Bind(wx.EVT_ERASE_BACKGROUND, self.OnErase)
		self.swap = True
		self.timer = wx.Timer(panel)
		panel.Bind(wx.EVT_TIMER, self.Swap, self.timer)
		self.timer.Start(100, False)
		
		return panel
		

	def OnErase(self, event):
		pass
		
	def Swap(self, event):
		self.Refresh()
		if self.swap:
			self.swap = False
		else:
			self.swap = True

	def OnPaint(self, event):
		if self.ready:
			dc = wx.BufferedPaintDC(self.main)
			dc.SetBackgroundMode(wx.SOLID)
			dc.SetBackground(wx.LIGHT_GREY_BRUSH)
			dc.Clear()

			#if self.ready:
			#	self.camera_image = wx.Image('capture.bmp', wx.BITMAP_TYPE_ANY).ConvertToBitmap()
			dc.DrawBitmap(self.camera_image, 10, 10)
		
	def CreateRegPanel(self, panel):
		colour2 = wx.Colour(red=200, green=200, blue=200)
		panel_sizer = wx.BoxSizer(wx.HORIZONTAL)
		
		pnl = wx.Panel(panel)
		sizer = wx.StaticBoxSizer(wx.StaticBox(pnl, wx.ID_ANY, 'Write Register'), wx.VERTICAL)
		szr = wx.FlexGridSizer(0, 2, 5, 10)
		lbl = wx.StaticText(pnl, label='Register:')
		txt = wx.TextCtrl(pnl, value='FF')
		self.reg = txt
		szr.Add(lbl, flag=wx.ALIGN_CENTER)
		szr.Add(txt, flag=wx.ALIGN_CENTER)
		lbl = wx.StaticText(pnl, label='Value:')
		txt = wx.TextCtrl(pnl, value='FF')
		self.val = txt
		szr.Add(lbl, flag=wx.ALIGN_CENTER)
		szr.Add(txt, flag=wx.ALIGN_CENTER)
		btn = wx.Button(pnl, wx.ID_ANY, 'Write Register')
		szr.Add(btn, flag=wx.ALIGN_CENTER)
		self.Bind(wx.EVT_BUTTON, self.OnSendWrite, btn)
		
		btn = wx.Button(pnl, wx.ID_ANY, 'Read Register')
		szr.Add(btn, flag=wx.ALIGN_CENTER)
		self.Bind(wx.EVT_BUTTON, self.OnSendRead, btn)
		
		lbl2 = wx.StaticText(pnl, wx.ID_ANY, '-NA-')
		lbl2.SetWindowStyle(lbl.GetWindowStyle()|wx.BORDER_SUNKEN|wx.ST_NO_AUTORESIZE)
		lbl2.SetBackgroundColour(colour2)
		szr.Add(lbl2, flag=wx.ALIGN_CENTER)
		lbl2.SetLabel('            -NA-            ')
		self.reg_fb = lbl2
		sizer.Add(szr, flag=wx.GROW)
		pnl.SetSizerAndFit(sizer)
		panel_sizer.Add(pnl, flag=wx.GROW, border=10)
		panel.SetSizerAndFit(panel_sizer)
		return panel
		
	def OnSendWrite(self, event):
		try:
			reg = int(self.reg.GetValue().strip(), 16)
			val = int(self.val.GetValue().strip(), 16)
		except ValueError:
			self.reg_fb.SetLabel(' Value Error ')
			return
		if reg > 255 or val > 255:
			self.reg_fb.SetLabel(' Error: > 255 ')
			return
			
		tx_string = '^%02X=%02X' % (reg, val)
		if len(tx_string) == 6:
			self.tx_string = tx_string
			
	def OnSendRead(self, event):
		try:
			reg = int(self.reg.GetValue().strip(), 16)
		except ValueError:
			self.reg_fb.SetLabel(' Value Error ')
			return
		if reg > 255:
			self.reg_fb.SetLabel(' Error: > 255 ')
			return
			
		tx_string = '*%02X' % (reg)
		if len(tx_string) == 3:
			self.tx_string = tx_string
		
	def convert_rgb565_to_bmp(self, image, outfile):
		c = []
		for i in range(0, len(image), 2):
			tmp = ord(image[i+1]) << 8
			tmp += ord(image[i])
			c.append(tmp)

		bmp_hdr = []
		bmp_hdr.append('B')
		bmp_hdr.append('M')
		bmp_hdr.append(struct.pack('<L', 0))
		bmp_hdr.append(struct.pack('<H', 0))
		bmp_hdr.append(struct.pack('<H', 0))
		bmp_hdr.append(struct.pack('<L', 14+40+12))

		bmp_hdr.append(struct.pack('<L', 40))
		bmp_hdr.append(struct.pack('<l', 320))
		bmp_hdr.append(struct.pack('<l', 240))
		bmp_hdr.append(struct.pack('<H', 1))
		bmp_hdr.append(struct.pack('<H', 16))
		bmp_hdr.append(struct.pack('<L', 3))
		bmp_hdr.append(struct.pack('<L', 320*240*2))	# Image dimension
		bmp_hdr.append(struct.pack('<l', 3780))
		bmp_hdr.append(struct.pack('<l', 3780))
		bmp_hdr.append(struct.pack('<L', 0))
		bmp_hdr.append(struct.pack('<L', 0))

		bmp_hdr.append(struct.pack('<L', 0xF800))
		bmp_hdr.append(struct.pack('<L', 0x7E0))
		bmp_hdr.append(struct.pack('<L', 0x1F))

		bmp_data = []
		for i in c:
			bmp_data.append(struct.pack('<H', i))
		'''
		f = open(outfile, 'wb')
		for i in bmp_hdr:
			f.write('%s' % i)
		for i in bmp_data:
			f.write('%s' % i)
		f.close()
		'''
		bmp = ''.join(bmp_hdr) + ''.join(bmp_data)
		'''
		for i in bmp_hdr:
			bmp += '%s' % i
		for i in bmp_data:
			bmp += '%s' % i
		'''
		return bmp

	def Serial_Process(self):
		ser = serial.Serial('COM6', 460800, timeout=0.01)
		cnt = 0
		while not self.quit:
			if self.tx_string != '':
				print self.tx_string
				ser.write(bytes(self.tx_string))
				rdbk = ''
				while len(rdbk) < 5:
					data = ser.read(16)
					if data != '':
						rdbk += data
				self.reg_fb.SetLabel(' %s ' % rdbk)
				self.tx_string = ''
			else:
				ser.write('$')
				image = ''
				prev = time.time()
				timeout = 0
				while len(image) < (320*240*2)+3 and timeout < 500:
					data = ser.read(16384)
					if data != '':
						image += data
					else:
						if self.quit:
							break
					timeout = time.time() - prev
				if timeout < 500:
					cnt += 1
					cnt = cnt & 0xFF
					print len(image), cnt
					print repr(image[:5])
					print repr(image[-5:])
					image = image[:320*240*2]
					#image = image[:-3]
					self.ready = False
					im = self.convert_rgb565_to_bmp(image, 'capture.bmp')
					im = StringIO.StringIO(im)
					self.camera_image = wx.ImageFromStream(im, type=wx.BITMAP_TYPE_ANY).ConvertToBitmap()
					self.camera_image.SaveFile('capture.bmp', type=wx.BITMAP_TYPE_BMP)
					self.ready = True
				else:
					print 'Timeout' + ' %d' % len(image)
		
def TriggerEvent(event, obj):
	cmd_evt = wx.CommandEvent(event)
	cmd_evt.SetEventObject(obj)
	cmd_evt.SetId(obj.GetId())
	cmd = obj.GetEventHandler().ProcessEvent
	wx.CallAfter(cmd, cmd_evt)
	
	
class RedirectText(object):
	def __init__(self, aLogCtrl):
		self.out = aLogCtrl
	
	def write(self, string):
		wx.CallAfter(self.out.AppendText, string)


class BE_App(wx.App):
	def OnInit(self):
		sys.excepthook = self.ExceptionHandler
		#frame = MainFrame(None, title='%s - %s' % (doc_number, doc_rev))
		frame = MainFrame(None, title='%s' % (doc_desc))
		self.SetTopWindow(frame)
		frame.CenterOnScreen()
		frame.Show()
		
		return True
	
	def ExceptionHandler(self, err_type, err_value, trace_back):
		tb = traceback.format_exception(err_type, err_value, trace_back)
		msg = ''.join(tb)
		
		dlg = ExceptionDialog(None, msg)
		dlg.CenterOnParent()
		dlg.ShowModal()
		dlg.Destroy()
		

class ExceptionDialog(wx.Dialog):
	def __init__(self, parent, msg):
		super(ExceptionDialog, self).__init__(parent=parent, title='Exception Error')
		
		art = wx.ArtProvider()
		err_bitmap = art.GetBitmap(wx.ART_ERROR)
		
		vsizer = wx.BoxSizer(wx.VERTICAL)
		lbl = wx.StaticText(self, label='Error detected')
		font = lbl.GetFont()
		font.SetWeight(wx.FONTWEIGHT_BOLD)
		lbl.SetFont(font)
		vsizer.Add(lbl, flag=wx.GROW|wx.ALL, border=10)
		
		lbl = wx.TextCtrl(self, value=msg, style=wx.TE_MULTILINE|wx.TE_READONLY|wx.TE_NO_VSCROLL)
		msg_lines = msg.split('\n')
		lines = len(msg_lines) + 1
		line_len = 0
		for i in msg_lines:
			if line_len < len(i):
				line_len = len(i)
				x, y = lbl.GetTextExtent(i)
		try:
			size = (x+10, y*lines)
			lbl.SetMinSize(size)
		except:
			pass
		
		vsizer.Add(lbl, flag=wx.GROW|wx.ALL, border=10)
		
		lbl = wx.StaticText(self, label='Please report the above traceback to SimLife.')
		vsizer.Add(lbl, flag=wx.GROW|wx.ALL, border=10)
		
		hsizer = wx.BoxSizer(wx.HORIZONTAL)
		bitmap = wx.StaticBitmap(self, wx.ID_ANY, err_bitmap)
		hsizer.Add(bitmap, flag=wx.ALL, border=10)
		hsizer.Add(vsizer, flag=wx.GROW)
		
		vsizer = wx.BoxSizer(wx.VERTICAL)
		vsizer.Add(hsizer, flag=wx.GROW)
		btn = wx.Button(self, wx.ID_OK)
		vsizer.Add(btn, flag=wx.ALIGN_CENTER_HORIZONTAL|wx.ALL, border=10)
		
		self.SetSizerAndFit(vsizer)

class BE_AppException(wx.App):
	def OnInit(self):
		global exception
		dlg = ExceptionDialog(None, exception)
		self.SetTopWindow(dlg)
		dlg.CenterOnParent()
		dlg.ShowModal()
		dlg.Destroy()
		
		return True
	
	def ExceptionHandler(self, err_type, err_value, trace_back):
		tb = traceback.format_exception(err_type, err_value, trace_back)
		msg = ''.join(tb)
		
		dlg = ExceptionDialog(None, msg)
		dlg.CenterOnParent()
		dlg.ShowModal()
		dlg.Destroy()


def InstallThreadExcepthook():
	'''
	Workaround for sys.excepthook thread bug
	Call once from __main__ before creating any threads.
	If using psyco, call psyco.cannotcompile(threading.Thread.run)
	since this replaces a new-style class method
	'''
	init_old = threading.Thread.__init__
	def init(self, *args, **kwargs):
		init_old(self, *args, **kwargs)
		run_old = self.run
		def run_with_excepthook(*args, **kw):
			try:
				run_old(*args, **kw)
			except (KeyboardInterrupt, SystemExit):
				raise
			except:
				wx.CallAfter(sys.excepthook, *sys.exc_info())
		self.run = run_with_excepthook
	threading.Thread.__init__ = init
	

if __name__ == '__main__':
	InstallThreadExcepthook()
	#global exception
	try:
		app = BE_App(False)
		app.MainLoop()
	except:
		err_type, err_value, trace_back = sys.exc_info()
		tb = traceback.format_exception(err_type, err_value, trace_back)
		msg = ''.join(tb)
		exception = msg
		app = BE_AppException(True)
		app.MainLoop()
		