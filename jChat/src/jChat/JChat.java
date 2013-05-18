package jChat;

import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;

import javax.swing.DefaultListModel;
import javax.swing.JFrame;
import javax.swing.JList;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.border.EmptyBorder;

public class JChat extends JFrame {
	private static final long serialVersionUID = 1L;

	/* UI elements */
	private JPanel contentPanel;	
	private JScrollPane textScrollPanel, userScrollPanel, msgScrollPanel;
	private JTextArea textArea, msgArea;
	private DefaultListModel<String> userListModel;
	private JList<String> userList;

	/* UI constructor */
	private void JChatUI() {
		contentPanel = new JPanel();
		contentPanel.setBorder(new EmptyBorder(5, 5, 5, 5));
		contentPanel.setLayout(new GridBagLayout());
		
		GridBagConstraints gbc = new GridBagConstraints();
		gbc.insets = new Insets(2, 2, 2, 2);
		
		textArea = new JTextArea();
		textArea.setEditable(false);
		textArea.setLineWrap(true);
		textArea.setWrapStyleWord(false);
		textScrollPanel = new JScrollPane();
		textScrollPanel.setPreferredSize(new Dimension(400, 300));
		textScrollPanel.setAutoscrolls(true);
		textScrollPanel.getViewport().add(textArea);
		gbc.gridx = 0;
		gbc.gridy = 0;
		gbc.weightx = 0.9;
		gbc.weighty = 0.9;
		gbc.fill = GridBagConstraints.BOTH;
		contentPanel.add(textScrollPanel, gbc);
		
		userListModel = new DefaultListModel<String>();
		userList = new JList<String>(userListModel);
		userScrollPanel = new JScrollPane();
		userScrollPanel.setPreferredSize(new Dimension(150, 300));
		userScrollPanel.getViewport().add(userList);
		gbc.gridx = 1;
		gbc.gridy = 0;
		gbc.weightx = 0.1;
		contentPanel.add(userScrollPanel, gbc);
				
		msgArea = new JTextArea();
		msgArea.setLineWrap(true);
		msgArea.setWrapStyleWord(false);
		msgScrollPanel = new JScrollPane();
		msgScrollPanel.setPreferredSize(new Dimension(0, 20));
		msgScrollPanel.setAutoscrolls(true);
		msgScrollPanel.getViewport().add(msgArea);
		gbc.gridx = 0;
		gbc.gridy = 1;
		gbc.gridwidth = 2;
		gbc.weightx = 1.0;
		gbc.weighty = 0.0;
		gbc.fill = GridBagConstraints.HORIZONTAL;
		contentPanel.add(msgScrollPanel, gbc);
		
		setContentPane(contentPanel);
		
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		setTitle("JChat");		
		pack();
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

	public JChat() {
		JChatUI();
	}
}