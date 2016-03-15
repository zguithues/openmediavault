<?php
/**
 * This file is part of OpenMediaVault.
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2016 Volker Theile
 *
 * OpenMediaVault is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OpenMediaVault is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenMediaVault. If not, see <http://www.gnu.org/licenses/>.
 */
namespace OMV\System\Net;

require_once("openmediavault/functions.inc");

/**
 * This class provides a simple interface to handle Linux network interfaces.
 * @ingroup api
 */
class NetworkInterface {
	protected $name = "";
	protected $ifconfig = null;
	protected $ip = null;
	protected $ethtool = null;
	protected $regex = array(
		"ipv4" => '\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}',
		"ipv6" => '[a-f0-9:]+',
		"ipv6cidr" => '([a-f0-9:]+)\/(\d{1,2})',
		"state" => 'UP|DOWN|UNKNOWN'
	);

	/**
	 * Constructor
	 * @param name The network interface name, e.g. eth0, ethx, ...
	 */
	public function __construct($name) {
		$this->name = $name;
	}

	/**
	 * Get the network interface configuration
	 * @private
	 * @return None.
	 * @throw \OMV\ExecException
	 */
	private function getData($type) {
		switch ($type) {
		case "ifconfig":
			if (!is_null($this->ifconfig))
				return;
			$cmdArgs = array();
			$cmdArgs[] = escapeshellarg($this->getDeviceName());
			$cmd = new \OMV\System\Process("ifconfig", $cmdArgs);
			$cmd->setRedirect2to1(TRUE);
			$cmd->execute($output);
			$this->ifconfig = implode("|", $output);
			break;
		case "ip":
			if (!is_null($this->ip))
				return;
			$cmdArgs = array();
			$cmdArgs[] = "addr";
			$cmdArgs[] = "show";
			$cmdArgs[] = "dev";
			$cmdArgs[] = escapeshellarg($this->getDeviceName());
			$cmd = new \OMV\System\Process("ip", $cmdArgs);
			$cmd->setRedirect2to1(TRUE);
			$cmd->execute($output);
			foreach ($output as $outputk => &$outputv)
				$outputv = trim($outputv);
			$this->ip = implode("|", $output);
			break;
		case "ethtool":
			if (!is_null($this->ethtool))
				return;
			$cmdArgs = array();
			$cmdArgs[] = escapeshellarg($this->getDeviceName());
			$cmd = new \OMV\System\Process("ethtool", $cmdArgs);
			$cmd->setRedirect2to1(TRUE);
			$cmd->execute($output);
			foreach ($output as $outputk => &$outputv)
				$outputv = trim($outputv);
			$this->ethtool = implode("|", $output);
			break;
		}
	}

	/**
	 * Get the network interface name, e.g. eth0 or ethx.
	 * @return The network interface name.
	 */
	public function getDeviceName() {
		return $this->name;
	}

	/**
	 * Check whether the network interface exists.
	 * @return TRUE if the network interface exists, otherwise FALSE.
	 */
	public function exists() {
		try {
			$this->getData("ifconfig");
		} catch(Exception $e) {
			return FALSE;
		}
		return TRUE;
	}

	/**
	 * Get the network interface IPv4 address.
	 * @return The network interface IPv4 address, otherwise FALSE.
	 * @throw \OMV\ExecException
	 */
	public function getIP() {
		$this->getData("ifconfig");
		if (1 !== preg_match("/inet addr:\s*({$this->regex['ipv4']})/i",
		  $this->ifconfig, $matches))
			return FALSE;
		return $matches[1];
	}

	/**
	 * Get the network interface IPv6 address.
	 * @return The network interface IPv6 address, otherwise FALSE.
	 * @throw \OMV\ExecException
	 */
	public function getIP6() {
		$this->getData("ifconfig");
		if (1 !== preg_match("/inet6 addr:\s*({$this->regex['ipv6cidr']})".
		  "\s*Scope:Global/i", $this->ifconfig, $matches))
			return FALSE;
		return $matches[2];
	}

	/**
	 * Get the network interface mask.
	 * @return The network interface mask, otherwise FALSE.
	 * @throw \OMV\ExecException
	 */
	public function getMask() {
		$this->getData("ifconfig");
		if (1 !== preg_match("/Mask:\s*({$this->regex['ipv4']})/i",
		  $this->ifconfig, $matches))
			return FALSE;
		return $matches[1];
	}

	/**
	 * Get the network interface IPv6 mask/prefix length.
	 * @return The network interface IPv6 mask/prefix length as integer,
	 *   otherwise FALSE.
	 * @throw \OMV\ExecException
	 */
	public function getMask6() {
		$this->getData("ifconfig");
		if (1 !== preg_match("/inet6 addr:\s*({$this->regex['ipv6cidr']})".
		  "\s*Scope:Global/i", $this->ifconfig, $matches))
			return FALSE;
		return intval($matches[3]);
	}

	/**
	 * Get the network interface MAC address.
	 * @return The network interface MAC address, otherwise FALSE.
	 */
	public function getMAC() {
		$filename = sprintf("/sys/class/net/%s/address",
		  $this->getDeviceName());
		if (!file_exists($filename))
			return FALSE;
		return trim(file_get_contents($filename));
	}

	/**
	 * Get the network interface MTU.
	 * @return The network interface MTU, otherwise FALSE.
	 */
	public function getMTU() {
		$filename = sprintf("/sys/class/net/%s/mtu",
		  $this->getDeviceName());
		if (!file_exists($filename))
			return FALSE;
		return trim(file_get_contents($filename));
	}

	/**
	 * Get the network interface IPv4 default gateway.
	 * @return The interface default gateway, or FALSE on failure.
	 * @throw \OMV\ExecException
	 */
	public function getGateway() {
		$cmdArgs = array();
		$cmdArgs[] = "-4";
		$cmdArgs[] = "route";
		$cmdArgs[] = "show";
		$cmdArgs[] = "dev";
		$cmdArgs[] = escapeshellarg($this->getDeviceName());
		$cmd = new \OMV\System\Process("ip", $cmdArgs);
		$cmd->setRedirect2to1(TRUE);
		$cmd->execute($output);
		$output = implode("\n", $output);
		// Parse command output:
		// 192.168.178.0/24  proto kernel  scope link  src 192.168.178.21
		// default via 192.168.178.1
		$regex = sprintf('/default via\s+(%s)/im', $this->regex['ipv4']);
		if (1 !== preg_match($regex, $output, $matches))
			return FALSE;
		return $matches[1];
	}

	/**
	 * Get the network interface IPv6 default gateway.
	 * @return The interface default gateway, or FALSE on failure.
	 * @throw \OMV\ExecException
	 */
	public function getGateway6() {
		$cmdArgs = array();
		$cmdArgs[] = "-6";
		$cmdArgs[] = "route";
		$cmdArgs[] = "show";
		$cmdArgs[] = "dev";
		$cmdArgs[] = escapeshellarg($this->getDeviceName());
		$cmd = new \OMV\System\Process("ip", $cmdArgs);
		$cmd->setRedirect2to1(TRUE);
		$cmd->execute($output);
		$output = implode("\n", $output);
		// Parse command output:
		// fe80::/64  proto kernel  metric 256  mtu 1500 advmss 1440 hoplimit 4294967295
		// default via fe80::21e:13ff:fef9:af0  metric 1024  mtu 1500 advmss 1440 hoplimit 4294967295
		$regex = sprintf('/default via\s+(%s)\s+/im', $this->regex['ipv6']);
		if (1 !== preg_match($regex, $output, $matches))
			return FALSE;
		return $matches[1];
	}

	/**
	 * Get the network interface state.
	 * @return The network interface state, otherwise FALSE.
	 * @throw \OMV\ExecException
	 */
	public function getState() {
		$this->getData("ethtool");
		if (1 !== preg_match("/state {$this->regex['state']}/i",
		  $this->ethtool, $matches))
			return FALSE;
		return $matches[0];
	}

	/**
	 * Get the network interface link state.
	 * @return TRUE if link is established, otherwise FALSE.
	 */
	public function getLink() {
		if (FALSE === ($carrier = $this->getCarrier()))
			return FALSE;
		return (1 == $carrier) ? TRUE : FALSE;
	}

	/**
	 * Get the network interface statistics.
	 * @return The network interface statistics, otherwise FALSE.
	 * array(
	 *   rx_bytes => xxx,
	 *   rx_packets => xxx,
	 *   rx_errors => xxx,
	 *   rx_dropped => xxx,
	 *   rx_fifo_errors => xxx,
	 *   rx_frame_errors => xxx,
	 *   rx_compressed => xxx,
	 *   rx_multicast => xxx,
	 *   tx_bytes => xxx,
	 *   tx_packets => xxx,
	 *   tx_errors => xxx,
	 *   tx_dropped => xxx,
	 *   tx_fifo_errors => xxx,
	 *   tx_collisions => xxx,
	 *   tx_carrier_errors => xxx,
	 *   tx_compressed => xxx
	 * )
	 */
	public function getStatistics() {
		// Parse command output:
		// Inter-|   Receive                                                |  Transmit
		//  face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
		//     lo:  156976     947    0    0    0     0          0         0   156976     947    0    0    0     0       0          0
		//   eth0:162137708  168681    0    0    0     0          0         0 15969317  103565    0    0    0     0       0          0
		//   eth1:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
		$regex = sprintf('/^\s*%s:(.*)$/im', $this->getDeviceName());
		if (1 !== preg_match($regex, file_get_contents("/proc/net/dev"),
		  $matches))
			return FALSE;
		$data = preg_split("/[\s]+/", $matches[1]);
		return array(
			  "rx_bytes" => $data[0],
			  "rx_packets" => $data[1],
			  "rx_errors" => $data[2],
			  "rx_dropped" => $data[3],
			  "rx_fifo_errors" => $data[4],
			  "rx_frame_errors" => $data[5],
			  "rx_compressed" => $data[6],
			  "rx_multicast" => $data[7],
			  "tx_bytes" => $data[8],
			  "tx_packets" => $data[9],
			  "tx_errors" => $data[10],
			  "tx_dropped" => $data[11],
			  "tx_fifo_errors" => $data[12],
			  "tx_collisions" => $data[13],
			  "tx_carrier_errors" => $data[14],
			  "tx_compressed" => $data[15]
		  );
	}

	/**
	 * Get the network interface operation state.
	 * @return The network interface operation state, otherwise FALSE.
	 */
	public function getOperState() {
		$filename = sprintf("/sys/class/net/%s/operstate",
		  $this->getDeviceName());
		if (!file_exists($filename))
			return FALSE;
		return trim(file_get_contents($filename));
	}

	/**
	 * Get the network interface duplex value.
	 * @see https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-net
	 * @return The network interface duplex value as string, otherwise FALSE.
	 */
	public function getDuplex() {
		$filename = sprintf("/sys/class/net/%s/duplex",
		  $this->getDeviceName());
		if (!file_exists($filename))
			return FALSE;
		return trim(file_get_contents($filename));
	}

	/**
	 * Get the network interface speed value.
	 * @see https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-net
	 * @return The network interface speed as number in Mbits/sec,
	 *   otherwise FALSE.
	 */
	public function getSpeed() {
		$filename = sprintf("/sys/class/net/%s/speed",
		  $this->getDeviceName());
		if (!file_exists($filename))
			return FALSE;
		return intval(file_get_contents($filename));
	}

	/**
	 * Get the network interface physical link state.
	 * @see https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-net
	 * @return The network interface physical link state, otherwise FALSE.
	 */
	public function getCarrier() {
		$filename = sprintf("/sys/class/net/%s/carrier",
		  $this->getDeviceName());
		if (!file_exists($filename))
			return FALSE;
		return intval(file_get_contents($filename));
	}

	/**
	 * Check if the network interface is a VLAN (802.1q) device.
	 * @return TRUE if the network interface is a VLAN (802.1q) device,
	 *   otherwise FALSE.
	 */
	public function isVlan() {
		$regex = "/^(\S+\d+)\.(\d+)$/i";
		if (1 !== preg_match($regex, $this->getDeviceName(), $matches))
			return FALSE;
		return TRUE;
	}

	/**
	* Get VLAN ID of the network interface.
	* @return The VLAN id, otherwise FALSE.
	*/
	public function getVlanId() {
		$regex = "/^(\S+\d+)\.(\d+)$/i";
		if (1 !== preg_match($regex, $this->getDeviceName(), $matches))
			return FALSE;
		return !empty($matches[2]) ? intval($matches[2]) : FALSE;
	}
}