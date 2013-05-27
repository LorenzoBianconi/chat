package jChat;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;

import javax.swing.DefaultListModel;
import javax.swing.JFrame;
import javax.swing.JList;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.border.Border;
import javax.swing.border.EmptyBorder;
import javax.swing.border.TitledBorder;
import javax.swing.border.LineBorder;

import java.io.*;
import java.net.*;
import java.nio.*;

import jChat.JChatMessage.JChatMsgType;

/* Chat Message */
class JChatMessage {
	static final int CHAT_HEADER_LEN = 8;

	public enum JChatMsgType {
		CHAT_AUTH_REQ,
		CHAT_AUTH_REP,
		CHAT_DATA,
		CHAT_USER_SUMMARY;

		public int enumToInt() { return this.ordinal(); }
		public static JChatMsgType IntToEnum(int type) { 
			switch (type) {
			case 0:
				return JChatMsgType.CHAT_AUTH_REQ;
			case 1:
				return JChatMsgType.CHAT_AUTH_REP;
			case 2:
				return JChatMsgType.CHAT_DATA;
			case 3:
			default:
				return JChatMsgType.CHAT_USER_SUMMARY;
			}
		}
		public static boolean checkType(int type) {
			if (type >= 0 && type <= 3) 
				return true;
			else
				return false;
		}
	}
	public enum JChatAuthReply {
		AUTH_DEN,
		AUTH_SUCCESS;

		public int enumToInt() { return this.ordinal(); }
		public static JChatAuthReply IntToEnum(int type) { 
			switch (type) {
			default:
			case 0:
				return JChatAuthReply.AUTH_DEN;
			case 1:
				return JChatAuthReply.AUTH_SUCCESS;
			}
		}
	}

	/* Chat message parsing methods */
	private static void setChatHeader(ByteBuffer message, JChatMsgType type,
									  int len) {
		message.putInt(type.enumToInt());
		message.putInt(len);
	}
	
	private static void setNickInfo(ByteBuffer message, String nick,
									int nicklen) {
		message.putInt(nicklen);
		message.put(nick.getBytes());
	}
	
	private static void setData(ByteBuffer message, String data) {
		message.put(data.getBytes());
	}
	
	public static String makeMsg(JChatMsgType type, String nick, String data) {
		int dlen = 4 + nick.length() + data.length();
		byte[] buff = new byte[8 + dlen];
		ByteBuffer message = ByteBuffer.wrap(buff);
		
		message.clear();
		message.order(ByteOrder.BIG_ENDIAN);
		
		setChatHeader(message, type, dlen);
		setNickInfo(message, nick, nick.length());
		switch (type) {
		case CHAT_AUTH_REQ:
			break;
		case CHAT_DATA:
			setData(message, data);
			break;
		default:
			break;
		}
		return new String(buff);
	}
	
	public static void sendMsg(Socket sock, String nick, String data,
							   JChatMsgType type) {
		try {
			OutputStream stream = sock.getOutputStream();
			PrintStream os = new PrintStream(new BufferedOutputStream(stream));
			os.println(JChatMessage.makeMsg(type, nick, data));
			os.flush();
		} catch (IOException e) {}
	}
}

public class JChat extends JFrame {
	/* Chat User Info */
	class JChatUser {
		private String _username;
		private String _hostname;

		JChatUser() {
			try {
				_username = System.getProperty("user.name");
				_hostname = java.net.InetAddress.getLocalHost().getHostName();
			} catch (UnknownHostException e) {}
		}
		JChatUser(String username, String hostname) {
			_username = username;
			_hostname = hostname;
		}
		public String getUsername() { return _username; }
		public String getHostname() { return _hostname; }
	}

	/* Chat Reader */
	class JChatReader implements Runnable {	
		JChatReader() {	
			Thread reader = new Thread(this, "Reader Thread");
			reader.start();
		}
		/* Parse chat message payload */
		public void parseChatMsg(ByteBuffer buff, JChatMsgType type) {
			int nickLen = buff.getInt();
			byte[] nick = new byte[nickLen];

			buff.get(nick);
			switch (type) {
			case CHAT_AUTH_REP:
				int res = buff.getInt();
				JChatMessage.JChatAuthReply auth_reply =
						JChatMessage.JChatAuthReply.IntToEnum(res);
				if (auth_reply != JChatMessage.JChatAuthReply.AUTH_SUCCESS) {
					JOptionPane.showMessageDialog(_contentPanel,
							"Authentication failed", "Error",
							JOptionPane.ERROR_MESSAGE);
					System.exit(ABORT);
				}
				break;
			case CHAT_DATA:
				byte[] data = new byte[buff.remaining()];
				while (buff.remaining() > 0)
					buff.get(data);
				_textArea.append("<" + new String(nick) + "> " + new String(data) + "\n");
				_textArea.setCaretPosition(_textArea.getDocument().getLength());
				break;
			case CHAT_USER_SUMMARY:
				DefaultListModel<String> users = new DefaultListModel<String>();
				while (buff.remaining() > 0) {
					int ulen = buff.getInt();
					byte[] unick = new byte[ulen];
					buff.get(unick);
					users.addElement(new String(unick));
				}
				for (int i = 0; i < users.getSize(); i++) {
					String user = users.elementAt(i);
					if (_userListModel.indexOf(user) < 0) {
						/* new user */
						_textArea.append("*" + user + " has joined\n" );
						_textArea.setCaretPosition(_textArea.getDocument().getLength());
						_userListModel.addElement(user);
					}
				}
				for (int i = 0; i < _userListModel.getSize(); i++) {
					String user = _userListModel.elementAt(i);
					if (users.indexOf(user) < 0) {
						/* left user */
						_textArea.append("*" + user + " has left\n");
						_textArea.setCaretPosition(_textArea.getDocument().getLength());
						_userListModel.removeElement(user);
					}
				}
			default:
				break;
			}
		}
		
		public void run() {
			try {
				InputStream stream = _sock.getInputStream();
				InputStreamReader isr = new InputStreamReader(stream);
				BufferedReader ib = new BufferedReader(isr);			

				while (true) {
					char[] c_header = new char[JChatMessage.CHAT_HEADER_LEN];
					/* get chat header */
					ib.read(c_header);
					String cheader = new String(c_header); 
					ByteBuffer ChatHeader = ByteBuffer.wrap(cheader.getBytes());
					int type = ChatHeader.getInt();
					int datalen = ChatHeader.getInt();
					if (datalen < 0 || !JChatMsgType.checkType(type))
						continue;
					char[] c_data = new char[datalen];
					ib.read(c_data);
					ByteBuffer data = ByteBuffer.wrap(new String(c_data).getBytes());
					parseChatMsg(data, JChatMsgType.IntToEnum(type));
				}
			} catch (IOException e) {}
		}
	}
	
	private static final long serialVersionUID = 1L;
	JChatUser _user;
	String _nick;
	/* UI elements */
	private JPanel _contentPanel;
	private JScrollPane _textScrollPanel, _userScrollPanel, _msgScrollPanel;
	private JTextArea _textArea, _msgArea;
	private DefaultListModel<String> _userListModel;
	private JList<String> _userList;
	/* Network elements */
	private Socket _sock;
	JChatReader _reader;
	
	class SndListener implements KeyListener {
		public void keyReleased(KeyEvent e) {
			if (e.getKeyCode() == KeyEvent.VK_ENTER) {
				String msg = _msgArea.getText();
				if (msg.length() > 0) {
					JChatMessage.sendMsg(_sock, _nick,
										 msg.substring(0, msg.length() - 1),
										 JChatMsgType.CHAT_DATA);
					_textArea.append("<" + _nick + "> " + msg);
					_textArea.setCaretPosition(_textArea.getDocument().getLength());
				}
				_msgArea.setText("");
			}
		}
		public void keyPressed(KeyEvent e) {}
		public void keyTyped(KeyEvent e) {}
	}

	/* UI constructor */
	private void JChatUI() {
		Border border = new LineBorder(Color.GRAY);
		
		_contentPanel = new JPanel();
		_contentPanel.setBorder(new EmptyBorder(5, 5, 5, 5));
		TitledBorder titleBorder = new TitledBorder(border, "JChat");
		_contentPanel.setBorder(titleBorder);
		_contentPanel.setLayout(new GridBagLayout());
		
		GridBagConstraints gbc = new GridBagConstraints();
		gbc.insets = new Insets(2, 2, 2, 2);
		
		_textArea = new JTextArea();
		_textArea.setEditable(false);
		_textArea.setLineWrap(true);
		_textArea.setWrapStyleWord(false);
		_textScrollPanel = new JScrollPane();
		_textScrollPanel.setBorder(border);
		_textScrollPanel.setPreferredSize(new Dimension(400, 300));
		_textScrollPanel.getViewport().add(_textArea);
		gbc.gridx = 0;
		gbc.gridy = 0;
		gbc.weightx = 0.9;
		gbc.weighty = 0.9;
		gbc.fill = GridBagConstraints.BOTH;
		_contentPanel.add(_textScrollPanel, gbc);
		
		_userListModel = new DefaultListModel<String>();
		_userList = new JList<String>(_userListModel);
		_userScrollPanel = new JScrollPane();
		_userScrollPanel.setBorder(border);
		_userScrollPanel.setPreferredSize(new Dimension(150, 300));
		_userScrollPanel.getViewport().add(_userList);
		gbc.gridx = 1;
		gbc.gridy = 0;
		gbc.weightx = 0.1;
		_contentPanel.add(_userScrollPanel, gbc);
		
		_msgArea = new JTextArea();
		_msgArea.setLineWrap(true);
		_msgArea.setWrapStyleWord(false);
		_msgArea.addKeyListener(new SndListener());
		_msgScrollPanel = new JScrollPane();
		_msgScrollPanel.setBorder(border);
		_msgScrollPanel.setPreferredSize(new Dimension(0, 20));
		_msgScrollPanel.setAutoscrolls(true);
		_msgScrollPanel.getViewport().add(_msgArea);
		gbc.gridx = 0;
		gbc.gridy = 1;
		gbc.gridwidth = 2;
		gbc.weightx = 1.0;
		gbc.weighty = 0.0;
		gbc.fill = GridBagConstraints.HORIZONTAL;
		_contentPanel.add(_msgScrollPanel, gbc);
		
		setContentPane(_contentPanel);
		
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		setTitle("Simple Chat");		
		pack();
	}
	
	public JChat() {
		_user = new JChatUser();
		_nick = _user.getUsername() + "@" + _user.getHostname();

		/* create the GUI */
		JChatUI();
		/* Connect to the server */
		try {
			_sock = new Socket("lorenet.dyndns.org", 9999);
			JChatReader reader = new JChatReader();
			
			/* Plain Authentication */
			JChatMessage.sendMsg(_sock, _nick, "", JChatMsgType.CHAT_AUTH_REQ);
		} catch (IOException e) {
			JOptionPane.showMessageDialog(_contentPanel,
					"Error connecting to ther server", "Error",
					JOptionPane.ERROR_MESSAGE);
			System.exit(ABORT);
		}
	}

	public static void main(String[] args) {
		EventQueue.invokeLater(new Runnable() {
			public void run() {
				try {
					JChat chat = new JChat();
					chat.setVisible(true);
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		});
	}
}